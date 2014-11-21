#ifndef __H_WATCHDOG_STATTOOL_H__
#define __H_WATCHDOG_STATTOOL_H__

#include "watchdog_daemon.h"

class CWatchDogStatTool: public CWatchDogDaemon
{
public:
	CWatchDogStatTool(CWatchDog *o, int sec);
	virtual ~CWatchDogStatTool(void);
	virtual void Exec(void);
};

#endif
