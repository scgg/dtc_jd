#ifndef __FULL_SYNC_H
#define __FULL_SYNC_H

#include <strings.h>
#include "async_file.h"
#include "ttcapi.h"

/*
 * 从master拉取limit行数据, 同步到slave
 */
class CFullSync {
public:
	CFullSync(TTC::Server * m, TTC::Server * s):
	    _master(m), _slave(s), _start(0), _limit(20) {
		bzero(_errmsg, sizeof(_errmsg));
	} ~CFullSync() {
	}

	int Init();

	int Run();

	void SetLimit(int l) {
		if (l <= 0 || l >= 10000)
			l = 10000;
		_limit = l;
	}

	const char *ErrorMessage() {
		return _errmsg;
	}

private:
	TTC::Server * _master;
	TTC::Server * _slave;
	int _start;
	int _limit;
	char _errmsg[256];

	//必须记住全量同步的状态
	CAsyncFileController _controller;
};
#endif
