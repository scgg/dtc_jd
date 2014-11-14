#ifndef __H_PROCESS_HELPER_H__
#define __H_PROCESS_HELPER_H__

#include "watchdog_daemon.h"
#include <string>

class CProcessHelper: public CWatchDogDaemon
{
private:
	std::string m_cmdLine;
	uint32_t    m_nArgc;
	uint32_t    m_nId;
	int         m_nPeriod;
	char       *m_argv[9];
public:
	CProcessHelper(CWatchDog *o, int sec, uint32_t id, std::string cmdLine);
	virtual ~CProcessHelper(void);
	virtual void Exec(void);
	int CommandlineToArgv(char *str, const char *delimiter, char *argv[], int limit);
};

#endif
