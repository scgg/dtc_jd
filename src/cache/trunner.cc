#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <getopt.h>

#include "thread.h"
#include "trunner.h"
#include "log.h"

/*
 * This is one-time test runner, memory leak and
 * all other dirty behavior is allowed
 */

class CRunner: public CThread {
	public:
		int argc;
		char *argv[100];

		CRunner() : CThread("trunner", 2), argc(0) {}
		virtual ~CRunner() {}
		virtual void *Process(void);
};

void *CRunner::Process(void) {
	void *lib;

	sleep(1);

	lib = dlopen("../lib/libttc.so", RTLD_NOW|RTLD_GLOBAL);
	if(lib==NULL) {
		log_error("Loading libttc.so failed: %s", dlerror());
		return NULL;
	}
	log_info("shared library %s loaded\n", "libttc.so");

	lib = dlopen(argv[0], RTLD_NOW);
	if(lib==NULL) {
		log_error("Loading %s failed: %s", argv[0], dlerror());
		return NULL;
	}

	log_info("testing shared object %s loaded\n", argv[0]);
	int (*entry)(int, char**) = (int (*)(int, char**)) dlsym(lib, "main");
	if(entry == NULL) {
		log_error("No main() found inside %s", argv[0]);
		return NULL;
	}

	optind = 0;
	optarg = NULL;
	log_info("main() entry is %p", entry);
	entry(argc, argv);
	return NULL;
}

void test_runner(void) {
	char *saveptr = NULL;

	// assume not null
	char *p = strdup(getenv("TTCD_TEST_RUNNER"));
	char *got;
	CRunner *runner = new CRunner;

	// assume never overflow
	while((got=strtok_r(p, " \t", &saveptr))) {
		runner->argv[runner->argc++] = got;
		p = NULL;
	}
	runner->argv[runner->argc] = NULL;

	if(runner->argc==0) return;
	runner->InitializeThread();
	runner->RunningThread();
}
