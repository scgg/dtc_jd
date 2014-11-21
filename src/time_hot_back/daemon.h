#ifndef __DAEMON_H__
#define __DAEMON_H__

#include "log.h"

class DaemonBase {
public:
	static int EnableFdLimit(int maxfd);
	static int EnableCoreDump();

	static int DaemonStart(int back = 1);
	static int DaemonWait();

private:
	 DaemonBase() {
	} virtual ~ DaemonBase() {
	}
	DaemonBase(const DaemonBase &);
	const DaemonBase & operator=(const DaemonBase &);

	static void sig_handler(int signal) {
		_stop = 1;
	}

public:
	static volatile int _stop;
};

#endif
