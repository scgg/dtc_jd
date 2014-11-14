#include "process_helper.h"
#include "log.h"

CProcessHelper::CProcessHelper(CWatchDog *o, int sec, uint32_t id, std::string cmdLine)
	: CWatchDogDaemon(o, sec)
{
	m_nId = id;
	m_cmdLine = cmdLine;
	
	m_nArgc = CommandlineToArgv(strdup(m_cmdLine.c_str()), " ", m_argv, 9);
	
	for (int i = 0; i < m_nArgc; i++)
	{
		log_info("argc:%d argv[%d]:%s", m_nArgc, i, m_argv[i]);
	}
	m_nPeriod = sec;
}

CProcessHelper::~CProcessHelper(void)
{
}

void CProcessHelper::Exec(void)
{	
	extern char** environ;
	log_debug("exec id:%d command line:%s argc:%d period:%d", m_nId, m_cmdLine.c_str(), m_nArgc, m_nPeriod);
	execve(m_argv[0], m_argv, environ);
}

int CProcessHelper::CommandlineToArgv(char *str, const char *delimiter, char *argv[], int limit)
{
    int i;
    int argc = 0;
    char *p;
    const char *ps = delimiter;

    if (ps == NULL)
        ps = " \t\r\n";

    p = str + strspn(str, ps);

    argv[0] = strtok(p, ps);
    if (argv[0] != NULL) {
        for (i = 1; (p = strtok(NULL, ps)) != NULL; ++i) {
            if (i >= limit) {
                break;
            }
            argv[i] = p;
        }
        argc = i;
    }

    return argc;
}