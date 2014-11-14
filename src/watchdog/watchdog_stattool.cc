#include <unistd.h>

#include "watchdog_stattool.h"

CWatchDogStatTool::CWatchDogStatTool(CWatchDog *o, int sec)
	: CWatchDogDaemon(o, sec)
{
	strncpy(name, "stattool", sizeof(name));
}

CWatchDogStatTool::~CWatchDogStatTool(void)
{
}

void CWatchDogStatTool::Exec(void)
{
	char *argv[3];

	argv[1] = (char *)"reporter";
	argv[2] = NULL;

	argv[0] = (char *)"stattool32";
	execv(argv[0], argv);
	argv[0] = (char *)"../bin/stattool32";
	execv(argv[0], argv);
	argv[0] = (char *)"stattool";
	execv(argv[0], argv);
	argv[0] = (char *)"../bin/stattool";
	execv(argv[0], argv);
}
