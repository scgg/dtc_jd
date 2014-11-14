#include "watchdog.h"
#include "process_helper.h"
#include "tinyxml.h"
#include "log.h"
#include "daemon.h"

char gCrontabConfig[256] = "../conf/crontab.xml";

const char progname[] = "dtccrontab";
const char usage_argv[] = "";
char cacheFile[256] = "";
char tableFile[256] = "";

int LoadConfig(CWatchDog *o)
{
	TiXmlDocument configdoc;
    if(!configdoc.LoadFile(gCrontabConfig))
    {
        log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gCrontabConfig, configdoc.ErrorDesc(), configdoc.ErrorRow(),
                configdoc.ErrorCol());
        return -1;
    }

    TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *modules = rootnode->FirstChildElement("PROCESS_LIST");
	

    TiXmlElement *m = modules->FirstChildElement("PROCESS");
    while(m)
    {
        uint32_t id = atoi(m->Attribute("Id"));
        std::string cmdLine = m->Attribute("CommandLine");
        int period = atoi(m->Attribute("Period"));

        log_debug("process id:%d command line:%s period:%d", id, cmdLine.c_str(), period);

        CProcessHelper *p = new CProcessHelper(o, period, id, cmdLine);

        if(p->Fork() < 0)
		{
			return -1;
		}

        m = m->NextSiblingElement("PROCESS");
    }

    return 0;
}

int main(int argc, char *argv[])
{
	DaemonStart();
	CWatchDog *wdog = new CWatchDog;
	
	_init_log_("dtccrontab", "../log");
	_set_log_level_(7);

	if (LoadConfig(wdog) < 0)
	{
		printf("dtc crontab load config fail\n");
		log_error("dtc crontab load config fail\n");
		return -1;
	}

	wdog->RunLoop();
	log_info("dtcrontab exit");

	return 0;
}
