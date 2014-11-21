#include "watchdog.h"
#include "process_helper.h"
#include "tinyxml.h"
#include "log.h"
#include "daemon.h"
#include <sys/mman.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <memcheck.h>
#include <errno.h>
#include <signal.h>
#include "Version.h"

char gCrontabConfig[256] = "../conf/crontab.xml";

const char progname[] = "dtccrontab";
const char usage_argv[] = "";
char cacheFile[256] = "";
char tableFile[256] = "";
pthread_mutex_t wakeLock;
int  logLevel = 7;


class CForkListener:
	public CPollerObject
{
private:
	CWatchDog *owner;
	CProcessHelper *list;
	int wakefd;

public:
	CForkListener(CWatchDog *o, CProcessHelper *list): owner(o), wakefd(-1){
		this->list = list;
	}
	
	virtual ~CForkListener(void){
		CProcessHelper *t = this->list;
		while(t)
		{
			CProcessHelper *temp = t;
			t = t->m_pNext;
			
			delete temp;
		}
	}
	
	int AttachWatchDog(void){
		int fd[2];
		int ret = pipe(fd);
        if(ret != 0)
            return ret;

		wakefd = fd[1];
		netfd = fd[0];
		EnableInput();
		owner->SetListener(this);
		
		return 0;
	}
	
	virtual void InputNotify(void){
		char buf[1];
		int n;
		n = read(netfd, buf, sizeof(buf));
		log_debug("read from pipe bytes:%d", n);
		ForkAllImp();
	}
	
	void ForkAll()
	{
		char c;
		log_debug("write pipe");
		write(wakefd, &c, 1);
		
	}
	
	void ForkAllImp()
	{
		log_debug("fork processes enter");
		CProcessHelper *t = this->list;
		while(t)
		{
			if (t->Fork() < 0)
			{
				log_error("dtc fork error %s\n", t->Dump().c_str());
			}
			t = t->m_pNext;
		}
	}
};

CForkListener *foker = NULL;

int LoadConfig(CWatchDog *o)
{
	CProcessHelper *list = NULL;
	TiXmlDocument configdoc;
    if(!configdoc.LoadFile(gCrontabConfig))
    {
        log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gCrontabConfig, configdoc.ErrorDesc(), configdoc.ErrorRow(),
                configdoc.ErrorCol());
        return -1;
    }

    TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *modules = rootnode->FirstChildElement("PROCESS_LIST");
	char *szLogLevel = modules->Attribute("LogLevel");
	if (szLogLevel)
	{
		logLevel = atoi(szLogLevel);
	}
    TiXmlElement *m = modules->FirstChildElement("PROCESS");
    while(m)
    {
        uint32_t id = atoi(m->Attribute("Id"));
        std::string cmdLine = m->Attribute("CommandLine");
        int period = atoi(m->Attribute("Period"));

        log_debug("process id:%d command line:%s period:%d", id, cmdLine.c_str(), period);

        CProcessHelper *p = new CProcessHelper(o, period, id, cmdLine);
		p->m_pNext = list;
		list = p;
        m = m->NextSiblingElement("PROCESS");
    }
	
	foker = new CForkListener(o, list);
	foker->AttachWatchDog();

    return 0;
}

void *__threadentry(void *p)
{
	sigset_t sset;
	sigfillset(&sset);
	sigdelset(&sset, SIGSEGV);
	sigdelset(&sset, SIGBUS);
	sigdelset(&sset, SIGABRT);
	sigdelset(&sset, SIGILL);
	sigdelset(&sset, SIGCHLD);
	sigdelset(&sset, SIGFPE);
	pthread_sigmask(SIG_BLOCK, &sset, &sset);
	
	time_t next = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	next = (tv.tv_sec / 10) * 10 + 10;
	struct timespec ts;
	ts.tv_sec = next;
	ts.tv_nsec = 0;

	while(pthread_mutex_timedlock(&wakeLock, &ts)!=0)
	{
		gettimeofday(&tv, NULL);
		if(tv.tv_sec >= next)
		{
			foker->ForkAll();
			gettimeofday(&tv, NULL);
			next = (tv.tv_sec / 10) * 10 + 10;
		}
		ts.tv_sec = next;
		ts.tv_nsec = 0;
	}
	pthread_mutex_unlock(&wakeLock);
	return NULL;
}

int main(int argc, char *argv[])
{
	int c = -1;
	while((c = getopt(argc, argv, "v")) != -1)
	{
		switch(c)
		{
			case 'v':
				printf("dtccrontab version: %s\n", GetCurrentVersion().c_str());
				printf("dtccrontab last commit date: %s\n", GetLastModifyDate().c_str());
				printf("dtccrontab last commit author: %s\n", GetLastAuthor().c_str());
				printf("dtccrontab compile date: %s\n", GetCompileDate().c_str());
				exit(0);
		}
	}
	
	DaemonStart();
	CWatchDog *wdog = new CWatchDog;
	pthread_t threadid;
	pthread_mutex_init(&wakeLock, NULL);
	LoadConfig(wdog);
	_init_log_("dtccrontab", "../log");
	_set_log_level_(logLevel);
	
	if(pthread_mutex_trylock(&wakeLock) == 0)
	{
		int ret = pthread_create(&threadid, 0, __threadentry, (void *)NULL);
		if(ret != 0)
		{
			errno = ret;
			return -1;
		}
	}

	wdog->RunLoop();
	
	if (foker)
		delete foker;
	log_info("dtcrontab exit");

	return 0;
}
