#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <version.h>
#include <asm/unistd.h>

#include "compiler.h"
#include "fault.h"

__HIDDEN
int CFaultHandler::DogPid = 0;

__HIDDEN
CFaultHandler CFaultHandler::Instance;

extern "C"
__attribute__((__weak__))
void CrashHook(int signo);

extern "C" {
__EXPORT volatile int crash_continue;
};

static int __crash_hook = 0;

__HIDDEN
void CFaultHandler::Handler(int signo, siginfo_t *dummy, void *dummy2) {
	signal(signo, SIG_DFL);
	if(DogPid > 1) {
		sigval tid;
		tid.sival_int = syscall(__NR_gettid);
		sigqueue(DogPid, SIGWINCH, tid);
		for(int i=0; i<50 && !crash_continue; i++)
			usleep(100*1000);
		if((__crash_hook != 0) && (&CrashHook != 0))
			CrashHook(signo);
	}
}

__HIDDEN
CFaultHandler::CFaultHandler(void) {
	Initialize(1);
}

__HIDDEN
void CFaultHandler::Initialize(int protect) {
	char *p = getenv(ENV_FAULT_LOGGER_PID);
	if(p && (DogPid = atoi(p)) > 1) {
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = CFaultHandler::Handler;
		sa.sa_flags = SA_RESTART|SA_SIGINFO;
		sigaction(SIGSEGV, &sa, NULL);
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGILL, &sa, NULL);
		sigaction(SIGABRT, &sa, NULL);
		sigaction(SIGFPE, &sa, NULL);
		if(protect)__crash_hook = 1;
	}
}

#if __pic__ || __PIC__
extern "C"
const char __invoke_dynamic_linker__[]
__attribute__ ((section (".interp")))
__HIDDEN
=
#if __x86_64__
	"/lib64/ld-linux-x86-64.so.2"
#else
	"/lib/ld-linux.so.2"
#endif
	;

extern "C"
__HIDDEN
int _so_start(void) {
#define BANNER "TTC FaultLogger v" TTC_VERSION_DETAIL "\n" \
	"  - USED BY TTCD INTERNAL ONLY!!!\n"
	int unused;
	unused = write(1, BANNER, sizeof(BANNER)-1);
	_exit(0);
}
#endif
