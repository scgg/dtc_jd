#ifndef __INCREMENTAL_SYNC_VALUE_H
#define __INCREMENTAL_SYNC_VALUE_H

#include "daemon.h"
#include "ttcapi.h"
#include "value.h"
#include "async_file.h"

class CIncSyncValue {
public:
	CIncSyncValue():
	    _master(0), _slave(0), _logReader(0), _type(0), _flag(0) {
		bzero(_errmsg, sizeof(_errmsg));
	} ~CIncSyncValue() {
	}

	int Init(TTC::Server * m, TTC::Server * s);
	int Run();
	int PurgeAllLocalDirtyKeys();
	const char *ErrorMessage() {
		return _errmsg;
	}

private:
	int FetchRawData(int);
	int ReplaceRawData(int);
	int PurgeDirtyKey(int);
	int AdjustSlaveLRU(int);
	int LoopAdjustSlaveLRU();
	int LoopUpdateSlaveData();
	int LoopPurgeDirtyKey();
	void FatalError(const char *);

private:
	TTC::Server * _master;
	TTC::Server * _slave;
	CAsyncFileReader *_logReader;
	char _errmsg[256];

	//ttc result
	TTC::Result _replace_result;
	TTC::Result _fetch_result;
	TTC::Result _purge_result;
	TTC::Result _lru_result;

	//decode buffer
	buffer _buff;

	//decode filed: type, flag, key, value
	unsigned _type;
	unsigned _flag;
	CBinary _key;
	CBinary _value;
};

#endif
