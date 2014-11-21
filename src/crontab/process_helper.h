#ifndef __H_PROCESS_HELPER_H__
#define __H_PROCESS_HELPER_H__

#include "watchdog.h"
#include <string>

class CProcessHelper: public CWatchDogObject
{
private:
	std::string m_cmdLine;
	uint32_t    m_nArgc;
	uint32_t    m_nId;
	int         m_nPeriod;
	char       *m_argv[9];
public:
	CProcessHelper *m_pNext;
public:
	CProcessHelper(CWatchDog *o, int sec, uint32_t id, std::string cmdLine);
	virtual ~CProcessHelper(void);
	void Exec(void);
	int CommandlineToArgv(char *str, const char *delimiter, char *argv[], int limit);
	std::string Dump();
private:
	virtual void KilledNotify(int,int);
	virtual void ExitedNotify(int);
public:
	int Fork(void);
};

#endif
