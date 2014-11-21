#include <sys/un.h>
#include "unix_socket.h"

#include "dbconfig.h"
#include "thread.h"
#include "watchdog_helper.h"
#include "daemon.h"
#include "log.h"
#include "ttc_global.h"
#include <sstream>

CWatchDogHelper::CWatchDogHelper(CWatchDog *o, int sec, const char *p, int g, int r, int b, int t)
	: CWatchDogDaemon(o, sec)
{
	/*snprintf(name, 20, "helper%u%c", g, MACHINEROLESTRING[r]);*/
	std::stringstream oss;
	oss << "helper" <<  g << MACHINEROLESTRING[r] ;
	memcpy(name, oss.str().c_str(), oss.str().length());
	path = p;
	backlog = b;
	type = t;
}

CWatchDogHelper::~CWatchDogHelper(void)
{
}

const static char *HelperName[] = 
{
	NULL,
	NULL,
	"mysql-helper",
	"tdb-helper",
	"custom-helper",
};

void CWatchDogHelper::Exec(void)
{
	struct sockaddr_un unaddr;
	int len = InitUnixSocketAddress(&unaddr, path);
	int listenfd = socket(unaddr.sun_family, SOCK_STREAM, 0);
	bind(listenfd, (sockaddr *)&unaddr, len);
	listen(listenfd, backlog);

	// relocate listenfd to stdin
	dup2(listenfd, 0);
	close(listenfd);

	char *argv[9];
	int argc = 0;

	argv[argc++] = NULL;
	argv[argc++] = (char *)"-d";
	if(strcmp(cacheFile, CACHE_CONF_NAME))
	{
		argv[argc++] = (char *)"-f";
		argv[argc++] = cacheFile;
	}
	if(strcmp(tableFile, TABLE_CONF_NAME))
	{
		argv[argc++] = (char *)"-t";
		argv[argc++] = tableFile;
	}
	argv[argc++] = name + 6;
	argv[argc++] = (char *)"-";
	argv[argc++] = NULL;

	CThread *helperThread = new CThread(name, CThread::ThreadTypeProcess);
	helperThread->InitializeThread();
	argv[0] = (char *)HelperName[type];
	execv(argv[0], argv);
	log_error("helper[%s] execv error: %m", argv[0]);
}

int CWatchDogHelper::Verify(void)
{
	struct sockaddr_un unaddr;
	int len = InitUnixSocketAddress(&unaddr, path);

	// delay 100ms and verify socket
	usleep(100*1000);
	int s = socket(unaddr.sun_family, SOCK_STREAM, 0);
	if(connect(s, (sockaddr *)&unaddr, len) < 0)
	{
		close(s);
		log_error("verify connect: %m");
		return -1;
	}
	close(s);

	return pid;
}
