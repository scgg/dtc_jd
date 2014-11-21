#ifndef __H_WATCHDOG_HELPER_H__
#define __H_WATCHDOG_HELPER_H__

#include "watchdog_daemon.h"



class CWatchDogHelper: public CWatchDogDaemon
{
private:
	const char *path;
	int backlog;
	int type;
public:
	CWatchDogHelper(CWatchDog *o, int sec, const char *p, int, int, int, int t);
	virtual ~CWatchDogHelper(void);
	virtual void Exec(void);
	int Verify(void);
};

#endif
