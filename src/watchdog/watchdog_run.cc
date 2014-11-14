#include <signal.h>

#include "watchdog.h"
#include "watchdog_main.h"
#include "watchdog_stattool.h"
#include "watchdog_listener.h"
#include "watchdog_helper.h"
#include "watchdog_logger.h"
#include "dbconfig.h"
#include "log.h"
#include "daemon.h"

int StartWatchDog(int (*entry)(void *), void *args)
{
	int delay = 5;
	dbConfig->SetHelperPath(getpid());
	CWatchDog *wdog = NULL;
	CWatchDogListener *srv = NULL;

	if(gConfig->GetIntVal ("cache", "DisableWatchDog", 0) == 0)
	{
		signal(SIGCHLD, SIG_DFL);

		wdog = new CWatchDog;
		srv = new CWatchDogListener(wdog);
		srv->AttachWatchDog();

		StartFaultLogger(wdog);

		delay = gConfig->GetIntVal ("cache", "WatchDogTime", 30);
		if(delay < 5) {
			delay = 5;
		}
		if(gConfig->GetIntVal ("cache", "StartStatReporter", 0) > 0)
		{
			CWatchDogStatTool *st = new CWatchDogStatTool(wdog, delay);
			if(st->Fork() < 0)
				return -1;	// cann't fork reporter
			log_info ("fork stat reporter");
		}
	}

    if(gConfig->GetIntVal("cache", "DisableDataSource", 0) == 0)
    {
        int nh = 0;
        /* starting master helper */
        for (int g = 0; g < dbConfig->machineCnt; g++)
        {
            for(int r=0; r<ROLES_PER_MACHINE; r++)
            {
                HELPERTYPE t = dbConfig->mach[g].helperType;

                log_debug ("helper type = %d", t);

                //check helper type is ttc
                if (TTC_HELPER >= t)
                {
                    break;
                }

                int i, n=0;
                for(i=0; i<GROUPS_PER_ROLE && (r*GROUPS_PER_ROLE+i)<GROUPS_PER_MACHINE; i++)
                {
                    n += dbConfig->mach[g].gprocs[r*GROUPS_PER_ROLE+i];
                }

                if(n <= 0)
                {
                    continue;
                }

                CWatchDogHelper *h = NULL;
                NEW (CWatchDogHelper(wdog, delay, dbConfig->mach[g].role[r].path, g, r, n+1, t), h);
                if (NULL == h)
                {
                    log_error ("create CWatchDogHelper object failed, msg:%m");
                    return -1;
                }

                if(h->Fork() < 0 || h->Verify() < 0)
                {
                    return -1;
                }

                nh++;
            }
        }

        log_info ("fork %d helper groups", nh);
    }

    if(wdog)
    {
	const int recovery = gConfig->GetIdxVal("cache", "ServerRecovery",
			((const char * const []){
			 "none",
			 "crash",
			 "crashdebug",
			 "killed",
			 "error",
			 "always",
			 NULL }), 2);

	CWatchDogEntry *ttc = new CWatchDogEntry(wdog, entry, args, recovery);

	if(ttc->Fork() < 0)
		return -1;

	wdog->RunLoop();
	exit(0);
    }

    return 0;
}

