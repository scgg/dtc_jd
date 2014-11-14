#include <unistd.h>
#include <stdlib.h>
#include "StatTTC.h"
#include "version.h"
#include "log.h"

CStatThread statmgr;

int InitStat(void)
{
	int ret;
	ret = statmgr.InitStatInfo("ttcd", STATIDX);
	// -1, recreate, -2, failed
	if(ret == -1)
	{
		unlink(STATIDX);
		char buf[64];
		ret = statmgr.CreateStatIndex("ttcd", STATIDX, StatDefinition, buf, sizeof(buf));
		if(ret != 0)
		{
			log_crit("CreateStatIndex failed: %s", statmgr.ErrorMessage());
			exit(ret);
		}
		ret = statmgr.InitStatInfo("ttcd", STATIDX);
	}
	if(ret==0)
	{
		int v1, v2, v3;
		sscanf(TTC_VERSION, "%d.%d.%d", &v1, &v2, &v3);
		statmgr.GetItem(S_VERSION) = v1*10000 + v2*100 + v3;
		statmgr.GetItem(C_TIME) = compile_time;
		_init_log_stat_();
	} else {
		log_crit("InitStatInfo failed: %s", statmgr.ErrorMessage());
		exit(ret);
	}
	return ret;
}

