#ifndef __H_WATCHDOG_DAEMON_H__
#define __H_WATCHDOG_DAEMON_H__

#include "watchdog.h"

class CWatchDogDaemon:
	public CWatchDogObject,
	private CTimerObject
{
private:
	CTimerList *timerList;

private:
	virtual void KilledNotify(int,int);
	virtual void ExitedNotify(int);
	virtual void TimerNotify(void);
public:
	CWatchDogDaemon(CWatchDog *o, int sec);
	~CWatchDogDaemon(void);

	int Fork(void);
	virtual void Exec(void) = 0;
};

#endif
