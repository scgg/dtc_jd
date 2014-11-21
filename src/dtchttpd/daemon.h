#include <unistd.h>

static volatile int httpdRunning = 1;

static void sigterm_handler(int signo)
{
    httpdRunning = 0;
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