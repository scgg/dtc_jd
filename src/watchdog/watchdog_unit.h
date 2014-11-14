#ifndef __H_WATCHDOG_UNIT_H__
#define __H_WATCHDOG_UNIT_H__
#include <stdlib.h>
#include <string.h>
#include <map>

class CWatchDogUnit;

class CWatchDogObject
{
protected:
	friend class CWatchDogUnit;
	CWatchDogUnit *owner;
	int pid;
	char name[16];

public:
	CWatchDogObject(void) : owner(NULL), pid(0) { name[0]='0';}
	CWatchDogObject(CWatchDogUnit *u) : owner(u), pid(0) { name[0]='0';}
	CWatchDogObject(CWatchDogUnit *u, const char *n) : owner(u), pid(0)
			{strncpy(name, n, sizeof(name));}
	CWatchDogObject(CWatchDogUnit *u, const char *n, int i) : owner(u), pid(i)
			{strncpy(name, n, sizeof(name));}
	virtual ~CWatchDogObject();
	virtual void ExitedNotify(int retval);
	virtual void KilledNotify(int signo, int coredumped);
	const char *Name(void) const { return name; }
	int Pid(void) const { return pid; }
	int AttachWatchDog(CWatchDogUnit *o=0);
	void ReportKillAlarm(int signo, int coredumped);
};

class CWatchDogUnit
{
private:
	friend class CWatchDogObject;
	typedef std::map<int, CWatchDogObject *> pidmap_t;
	int pidCount;
	pidmap_t pid2obj;

private:
	int AttachProcess(CWatchDogObject *);
public:
	CWatchDogUnit(void);
	virtual ~CWatchDogUnit(void);
	int CheckWatchDog(void); // return #pid monitored
	int KillAll(void);
	int ForceKillAll(void);
	int ProcessCount(void) const { return pidCount; }
};

#endif
