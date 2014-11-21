#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include <stdexcept>
#include "version.h"
#include "json/json.h"
#include "poll_thread_group.h"
#include "log.h"
#include "http_server.h"
#include "prober_config.h"
#include "prober_cmd.h"
#include "memory_usage_cmd.h"
#include "map_reduce.h"
#include "memcheck.h"
#include "RelativeHourCalculator.h"
#include "segvcatch.h"

static char prog_name[] = "dtcproberd";

extern pthread_mutex_t globalLock;

static volatile int running = 1;

static void sigterm_handler(int signo)
{
	running = 0;
}

static void daemon_start()
{
	struct sigaction sa;
	sigset_t sset;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	signal(SIGPIPE,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);

	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
}

int DaemonSetFdLimit(int maxfd)
{
	struct rlimit rlim;
	if(maxfd)
	{
		/* raise open files */
		rlim.rlim_cur = maxfd;
		rlim.rlim_max = maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
			log_error("Increase FdLimit failed, set val[%d] errmsg[%s]",maxfd,strerror(errno));
			return -1;
		}
	}
	return 0;
}

void ShowUsage(int argc, char **argv) {
	printf("%s [-c conf]\n", argv[0]);
	printf("    -c conf\n");
	printf("        config file, default ../conf/prober.xml\n");
}

void ShowVersion() {
	printf("%s version: %s %s\n", prog_name, version, git_version);
	printf("%s compile date: %s %s\n", prog_name, compdatestr, comptimestr);
}

void handle_segv() {
	//return;
	throw std::runtime_error("My SEGV");
}

void handle_fpe() {
	//return;
	throw std::runtime_error("My FPE");
}

int main(int argc, char ** argv)
{
	int c;
	char config_file[256] = "../conf/prober.xml";
	while ((c = getopt(argc, argv, "c:hv")) != -1)
	{
		switch (c)
		{
			case 'c':
				strncpy(config_file, optarg, sizeof(config_file) - 1);
				config_file[sizeof(config_file) - 1] = 0;
				break;
			case 'h':
				ShowUsage(argc, argv);
				exit(0);
			case 'v':
				ShowVersion();
				exit(0);
			case '?':
			default :
				ShowUsage(argc, argv);
				exit(-1);
		}
	}
 
	signal(SIGPIPE,SIG_IGN);
	//no change dir
	daemon(1, 0);

	_init_log_(prog_name, "../log/");
	CProberConfig proberConfig(config_file);
	if (proberConfig.Load()) {
		log_error("Load error");
		return -1;
	}

	segvcatch::init_segv(&handle_segv);
	segvcatch::init_fpe(&handle_fpe);

	int worker_num = proberConfig.WorkerNum();

	while(true)
	{
		pid_t p = fork();
		if(p == 0)
		{
			//if (1 != atoi(log_switch))
			daemon_start();
			DaemonSetFdLimit(102400);
			log_info("%s start...", prog_name);
			CPollThread pollThread(prog_name);
			pollThread.SetMaxPollers(100000);
			pollThread.InitializeThread();
			CPollThread *worker_thread[worker_num];
			CDispatcher *dispatcher[worker_num];
			CWorker *worker[worker_num];
			for (int i = 0; i < worker_num; ++i) {
				char buf[32];
				snprintf(buf, 32, "worker@%d", i);
				worker_thread[i] = new CPollThread(buf);
				worker_thread[i]->SetMaxPollers(10000);
				worker_thread[i]->InitializeThread();
				int fd[2];
				socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
				dispatcher[i] = new CDispatcher(&pollThread, fd[0]);
				worker[i] = new CWorker(worker_thread[i], fd[1]);
			}
			CHttpServer httpServer(&pollThread, &proberConfig, dispatcher);
			httpServer.ListenOn(proberConfig.ListenOn());
			pollThread.RunningThread();
			for (int i = 0; i < worker_num; ++i)
				worker_thread[i]->RunningThread();
			while (running)
				pause();
			log_info("%s stopped", prog_name);
			return 0;
		}
		else if (p > 0)
		{
			log_info("wait for child ...");
			int status;
			int ret = waitpid(p, &status, 0);
			while(ret < 0 && errno == EINTR)
			{
				ret = waitpid(p, &status, 0);
			}

			if(ret < 0)
			{
				log_error("waitpid error");
				return -1;
			}

			if(WIFEXITED(status))
			{
				if(WEXITSTATUS(status) == 0)
					return 0;
				else if((char)WEXITSTATUS(status) < 0)
					return WEXITSTATUS(status);
			}
			else if(WIFSIGNALED(status))
			{
				log_error("killed by signal %d", WTERMSIG(status));
			}
		}
		else
		{
			log_error("fork error");
			return -1;
		}
	}
}
