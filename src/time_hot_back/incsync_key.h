#ifndef __INCREMENTAL_SYNC_H
#define __INCREMENTAL_SYNC_H

#include "ttcapi.h"
#include "value.h"
#include "async_file.h"

/*
 * 增量同步变更key，lru列表到本地文件缓存
 */
class CIncSyncKey {
public:
	CIncSyncKey():_master(0),
	    _logWriter(0), _journal_id(), _type(0), _flag(0) {
		bzero(_errmsg, sizeof(_errmsg));
	}
       	~CIncSyncKey() {
	}

	int Run();
	int Init(TTC::Server * master, int max = 5);
	void SetLimit(int l) {
		if (l <= 0 || l >= 10000)
			l = 10000;
		_limit = l;
	}
	const char *ErrorMessage() {
		return _errmsg;
	}

private:
	void FatalError(const char *);

private:

	TTC::Server * _master;
	CAsyncFileWriter *_logWriter;

	CJournalID _journal_id;

	/*编解码用 */
	unsigned _type;
	unsigned _flag;
	CBinary _key;
	CBinary _value;
    CBinary _newvalue;

	//批量拉取更新key
	int _limit;

	char _errmsg[256];
};

#endif
