#ifndef __H_WATCHDOG_LISTENER_H__
#define __H_WATCHDOG_LISTENER_H__

#include "watchdog.h"

#define ENV_WATCHDOG_SOCKET_FD "WATCHDOG_SOCKET_FD"

class CWatchDogListener:
	public CPollerObject
{
private:
	CWatchDog *owner;
	int peerfd;

public:
	CWatchDogListener(CWatchDog *o);
	virtual ~CWatchDogListener(void);
	int AttachWatchDog(void);
	virtual void InputNotify(void);
};

extern int WatchDogFork(const char *name, int(*entry)(void *), void *args);
#endif
