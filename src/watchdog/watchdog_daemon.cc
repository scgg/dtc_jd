#include <fcntl.h>
#include <errno.h>

#include "watchdog_daemon.h"
#include "log.h"

CWatchDogDaemon::CWatchDogDaemon(CWatchDog *o, int sec)
	: CWatchDogObject(o)
{
	if(o)
		timerList = o->GetTimerList(sec);
}

CWatchDogDaemon::~CWatchDogDaemon(void)
{
}

int CWatchDogDaemon::Fork(void)
{
	// an error detection pipe
	int err, fd[2];
	int unused;

	unused = pipe(fd);

	// fork child process
	pid = fork();
	if (pid == -1)
		return pid;

	if (pid == 0)
	{
		// close pipe if exec succ
		close(fd[0]);
		fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		Exec();
		err = errno;
		log_error("%s: exec(): %m", name);
		unused = write(fd[1], &err, sizeof(err));
		exit(-1);
	}

	close(fd[1]);

	if(read(fd[0], &err, sizeof(err)) == sizeof(err))
	{
		errno = err;
		return -1;
	}
	close(fd[0]);

	AttachWatchDog();
	return pid;
}

void CWatchDogDaemon::TimerNotify(void)
{
	if(Fork() < 0)
		if(timerList)
			AttachTimer(timerList);
}

void CWatchDogDaemon::KilledNotify(int sig, int cd)
{
	if(timerList)
		AttachTimer(timerList);
	ReportKillAlarm(sig, cd);
}

void CWatchDogDaemon::ExitedNotify(int retval)
{
	if(timerList)
		AttachTimer(timerList);
}

