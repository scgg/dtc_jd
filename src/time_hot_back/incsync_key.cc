#include <errno.h>
#include "incsync_key.h"
#include "memcheck.h"
#include "value.h"
#include "daemon.h"
#include "log.h"
#include "comm.h"

int CIncSyncKey::Init(TTC::Server * master, int max)
{
	NEW(CAsyncFileWriter(max), _logWriter);
	if (!_logWriter) {
		snprintf(_errmsg, sizeof(_errmsg),
			 "create CAsyncFileWriter obj failed");
		return -1;
	}

	if (_logWriter->Open()) {
		snprintf(_errmsg, sizeof(_errmsg),
			 "CAsyncFileWriter open failed, %s",
			 _logWriter->ErrorMessage());

		return -1;
	}

	_master = master;
	_journal_id = _logWriter->JournalID();

	return 0;
}

//TODO: 时间不够，硬编码
#define ENCODE_FIELDS_TO_BUFF(b) do {\
	b.clear();  \
	b.append((const char *)&_journal_id, sizeof(CJournalID)); \
	b.append((const char *)&_type, 4);  \
	b.append((const char *)&_flag, 4);   \
	b.append((const char *)&(_key.len), 4); \
	b.append((const char *)_key.ptr, _key.len); \
	b.append((const char *)&(_value.len), 4); \
	b.append((const char *)_value.ptr, _value.len); \
}while(0)

#define DECODE_FIELDS_FROM_SVR(result) do{ \
	CBinary v; \
	_type = result.IntValue("type");\
	_flag = result.IntValue("flag");\
	v.ptr = (char *)result.BinaryValue("key",   v.len); _key   = v; \
	v.ptr = (char *)result.BinaryValue("value", v.len); _value = v; \
}while(0)

#define DECODE_JID_FROM_SVR(jid, result) do {\
	_journal_id  = (uint64_t) result.HotBackupID(); \
}while(0)

//TODO: 告警
//目前是记录一条错误日志，后续添加告警平台通知
//通常是不允许这类错误发生，一旦发生就只能手工处理了。
void CIncSyncKey::FatalError(const char *msg)
{
	CComm::registor.SetHBPStatusDirty();

	log_error("__ERROR__: %s, shutdown now", msg);
	exit(-1);
}

int CIncSyncKey::Run()
{
	int sec = 0;
	struct timeval now;

	gettimeofday(&now, NULL);
	sec = now.tv_sec + 1;

	/* 不允许直接kill */
	while (1) {
		gettimeofday(&now, NULL);
		if (now.tv_sec >= sec) {
			if (CComm::registor.CheckMemoryCreateTime()) {
				FatalError("detect share memory changed");
			}

			if (getppid() == 1) {
				return -1;
			}

			sec += 1;
		}

		TTC::SvrAdminRequest request(_master);
		request.SetAdminCode(TTC::GetUpdateKey);

		/* 手动设置Need字段 */
		request.Need("type");
		request.Need("flag");
		request.Need("key");
		request.Need("value");
		request.SetHotBackupID((uint64_t) _journal_id);

		/* 支持批量拉取更新key */
		request.Limit(0, _limit);

		TTC::Result result;

		/* 执行 */
		int ret = request.Execute(result);
		if (-EBADRQC == ret) {
			FatalError("master hot-backup not active yet");
		}

		if (-TTC::EC_BAD_HOTBACKUP_JID == ret) {
			FatalError("master report journalID is not match");

		}

		if (0 != ret) {
			log_debug
			    ("fetch update-key-list failed, retry, err: %s, from: %s, limit: %d",
			     result.ErrorMessage(), result.ErrorFrom(), _limit);
			usleep(100);
			continue;
		}

		/* decode and write to async_file */
		buffer buff;

		for (int i = 0; i < result.NumRows(); ++i) {
			result.FetchRow();

			DECODE_FIELDS_FROM_SVR(result);

			ENCODE_FIELDS_TO_BUFF(buff);

			if (_logWriter->Write(buff)) {
				//XXX help!
				FatalError(_logWriter->ErrorMessage());
			}
		}

		//更新控制文件中的journalID
		DECODE_JID_FROM_SVR(_journal_id, result);
		_logWriter->JournalID() = _journal_id;
	}

	return 0;
}
