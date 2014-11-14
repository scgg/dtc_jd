#include <sys/wait.h>
#include <errno.h>

#include "config.h"
#include "watchdog_unit.h"
#include "log.h"
 #include "DtcStatAlarmReporter.h"
 #include <sstream>
 #include <ttc_global.h>
CWatchDogObject::~CWatchDogObject(void)
{
}

int CWatchDogObject::AttachWatchDog(CWatchDogUnit *u)
{
	if(u && owner==NULL)
		owner = u;

	return owner ? owner->AttachProcess(this) : -1;
}

void CWatchDogObject::ExitedNotify(int retval)
{
	delete this;
}

void CWatchDogObject::KilledNotify(int sig, int cd)
{
	ReportKillAlarm(sig, cd);
	delete this;
}
void CWatchDogObject::ReportKillAlarm(int signo, int coredumped)
{
	
	if (!ALARM_REPORTER->InitAlarmCfg(std::string(ALARM_CONF_FILE) , true))
	{
		log_error("init alarm conf file fail");
		return;
	}
	ALARM_REPORTER->SetTimeOut(1);
	std::stringstream oss;
	oss << "child process["  << pid << "][ " << name << " ]killed by signal " << signo ;
	if(coredumped)
	{
		oss << " core dumped";
	}
	ALARM_REPORTER->ReportAlarm(oss.str());
}
CWatchDogUnit::CWatchDogUnit(void)
	: pidCount(0)
{
};

CWatchDogUnit::~CWatchDogUnit(void)
{
	pidmap_t::iterator i;
	for(i = pid2obj.begin(); i != pid2obj.end(); i++)
	{
		CWatchDogObject *obj = i->second;
		delete obj;
	}
};

int CWatchDogUnit::CheckWatchDog(void)
{
	while(1)
	{
		int status;
		int pid = waitpid(-1, &status, WNOHANG);
		int err = errno;

		if(pid < 0)
		{
			switch(err)
			{
				case EINTR:
				case ECHILD:
					break;
				default:
					log_notice("wait() return pid %d errno %d", pid, err);
					break;
			}
			break;
		} else if(pid==0) {
			break;
		} else
		{
			pidmap_t::iterator itr = pid2obj.find(pid);
			if(itr == pid2obj.end())
			{
				log_notice("wait() return unknown pid %d status 0x%x", pid, status);
			} else {
				CWatchDogObject *obj = itr->second;
				const char * const name = obj->Name();

				pid2obj.erase(itr);

				// special exit value return-ed by CrashProtector
				if(WIFEXITED(status) && WEXITSTATUS(status)==85 && strncmp(name, "main", 5)==0)
				{	// treat it as a fake SIGSEGV
					status = W_STOPCODE(SIGSEGV);
				}

				if(WIFSIGNALED(status))
				{
					const int sig = WTERMSIG(status);
					const int core = WCOREDUMP(status);

					log_alert("%s: killed by signal %d", name, sig);

					log_error("child %.16s pid %d killed by signal %d%s",
							name, pid, sig,
							core ? " (core dumped)" : "");
					pidCount--;
					obj->KilledNotify(sig, core);
				} else if(WIFEXITED(status))
				{
					const int retval = (signed char)WEXITSTATUS(status);
					if(retval==0)
						log_debug("child %.16s pid %d exit status %d",
								name, pid, retval);
					else
						log_info("child %.16s pid %d exit status %d",
								name, pid, retval);
					pidCount--;
					obj->ExitedNotify(retval);
				}
			}
		}
	}
	return pidCount;
}

int CWatchDogUnit::AttachProcess(CWatchDogObject *obj)
{
	const int pid = obj->pid;
	pidmap_t::iterator itr = pid2obj.find(pid);
	if(itr != pid2obj.end() || pid <= 1)
		return -1;

	pid2obj[pid] = obj;
	pidCount++;
	return 0;
}

int CWatchDogUnit::KillAll(void)
{
	pidmap_t::iterator i;
	int n = 0;
	for(i = pid2obj.begin(); i != pid2obj.end(); i++)
	{
		CWatchDogObject *obj = i->second;
		if(obj->Pid() > 1)
		{
			log_debug("killing child %.16s pid %d SIGTERM",
					obj->Name(), obj->Pid());
			kill(obj->Pid(), SIGTERM);
			n++;
		}
	}
	return n;
}

int CWatchDogUnit::ForceKillAll(void)
{
	pidmap_t::iterator i;
	int n = 0;
	for(i = pid2obj.begin(); i != pid2obj.end(); i++)
	{
		CWatchDogObject *obj = i->second;
		if(obj->Pid() > 1)
		{
			log_error("child %.16s pid %d didn't exit in timely, sending SIGKILL.",
				obj->Name(), obj->Pid());
			kill(obj->Pid(), SIGKILL);
			n++;
		}
	}
	return n;
}

