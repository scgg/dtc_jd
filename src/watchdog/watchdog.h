#ifndef __H_WATCHDOG__H__
#define __H_WATCHDOG__H__

#include "watchdog_unit.h"
#include "poller.h"
#include "timerlist.h"

class CWatchDogPipe: public CPollerObject
{
private:
	int peerfd;
public:
	CWatchDogPipe(void);
	virtual ~CWatchDogPipe(void);
	void Wake(void);
	virtual void InputNotify(void);
};

class CWatchDog:
	public CWatchDogUnit,
	public CTimerUnit
{
private:
	CPollerObject *listener;
public:
	CWatchDog(void);
	virtual ~CWatchDog(void);

	void SetListener(CPollerObject *l) { listener = l; }
	void RunLoop(void);
};

extern int StartWatchDog(int (*entry)(void *), void *);
#endif
