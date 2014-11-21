#ifndef __H_WATCHDOG_MAIN_H__
#define __H_WATCHDOG_MAIN_H__

#include "watchdog.h"

class CWatchDogEntry: private CWatchDogObject
{
private:
	int (*entry)(void *);
	void *args;
	int recovery;
	int corecount;

private:
	virtual void KilledNotify(int sig, int cd);
	virtual void ExitedNotify(int rv);

public:
	CWatchDogEntry(CWatchDogUnit *o, int (*entry)(void *), void *args, int r);
	virtual ~CWatchDogEntry(void);

	int Fork(int enCoreDump=0);
};

#endif
