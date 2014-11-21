#include <errno.h>
#include "comm.h"
#include "daemon.h"
#include "incsync_value.h"

int CIncSyncValue::Init(TTC::Server * m, TTC::Server * s)
{
	NEW(CAsyncFileReader, _logReader);

	if (!_logReader) {
		snprintf(_errmsg, sizeof(_errmsg),
			 "create CAsyncFileReader obj failed");
		return -1;
	}

	if (_logReader->Open()) {
		snprintf(_errmsg, sizeof(_errmsg),
			 "CAsyncFileReader open failed, %s",
			 _logReader->ErrorMessage());
		return -1;
	}

	_master = m;
	_slave = s;

	return 0;
}

/* 出现了不能自动恢复的错误 */
void CIncSyncValue::FatalError(const char *msg)
{
	/* 
	 * 设置hbp状态为 全量同步未完成
	 * hpb重启的时候，手动修复错误
	 *
	 */
	CComm::registor.SetHBPStatusDirty();

	log_error("__ERROR__: %s, shutdown now", msg);
	exit(-1);
}

#define DECODE_FIELDS_FROM_BUFF(buff) do { \
	const char *p = buff.c_str();\
	p += sizeof(CJournalID);  \
	_type = *(unsigned *)p; p+=4; \
	_flag = *(unsigned *)p; p+=4; \
	int l = *(int *)p; p+=4;  \
	_key.ptr = (char *)p; \
	_key.len = l; p+=l; \
	l = *(int *)p; p+=4; \
	_value.ptr = (char *)p; \
	_value.len = l; \
}while(0)

int CIncSyncValue::FetchRawData(int log)
{
	log_debug("hb fetch raw data from master");

	//从master拉取数据
	TTC::SvrAdminRequest request(_master);
	request.SetAdminCode(TTC::GetRawData);

	request.EQ("key", _key.ptr, _key.len);
	request.Need("type");
	request.Need("flag");
	request.Need("value");

	int ret = request.Execute(_fetch_result);
	if (-EBADRQC == ret) {
		FatalError("master hot-backup not active yet");
	}

	if (ret != 0) {
		if(log) {
			log_warning("fetch error, msg: %s, from:%s",
					_fetch_result.ErrorMessage(),
					_fetch_result.ErrorFrom()
				 );
		}

		return -1;
	}

	if (_fetch_result.NumRows() <= 0)
		return -1;

	_fetch_result.FetchRow();

	/* 解码 */
	_type = _fetch_result.IntValue("type");
	_flag = _fetch_result.IntValue("flag");
	_value.ptr = (char *)_fetch_result.BinaryValue("value", _value.len);

	return 0;
}

//替换备机数据
int CIncSyncValue::ReplaceRawData(int log)
{
	log_debug("hb replace raw data to slave");

	TTC::SvrAdminRequest request(_slave);
	request.SetAdminCode(TTC::ReplaceRawData);

	request.EQ("key", _key.ptr, _key.len);
	request.Set("type", _type);
	request.Set("flag", _flag);
	request.Set("value", _value.ptr, _value.len);

	int ret = request.Execute(_replace_result);
	if (-EBADRQC == ret) {
		FatalError("slave hot-backup not active yet");
	}

	if (ret != 0) {
		if(log) {
			log_warning("replace error, msg: %s, from:%s",
					_replace_result.ErrorMessage(),
					_replace_result.ErrorFrom());
		}

		return -1;
	}

	return 0;
}

int CIncSyncValue::LoopUpdateSlaveData()
{
	int ret=0;
	int log=1;

	// 没有附带value字段，重新拉取
	if (_flag == CHBGlobal::NON_VALUE) {
		while((ret=FetchRawData(log))) {
			if(log) {
				log_warning("fetch raw data from master error, enter loop retry");
			}

			if(getppid()==1){
				break;
			}

			log=0;
			usleep(READER_SLEEP_TIME);
		}

		//停机的时候，还有一个同步请求未处理完成
		if(ret)
		{
			FatalError("some request miss execute");
		}
	}


	log=1;
	while((ret=ReplaceRawData(log))) {
		if(log){
			log_warning("replace raw data to slave error, enter loop retry");
		}

		if(getppid()==1){
			break;
		}
		log=0;
		usleep(READER_SLEEP_TIME);
	}

	//停机的时候，还有一个同步请求未处理完成
	if(ret)
	{
		FatalError("some request miss execute");
	}

	return 0;
}

int CIncSyncValue::AdjustSlaveLRU(int log)
{
	log_debug("hb adjust slave lru");

	TTC::SvrAdminRequest request(_slave);
	request.SetAdminCode(TTC::AdjustLRU);

	request.EQ("key", _key.ptr, _key.len);

	int ret = request.Execute(_lru_result);
	if (-EBADRQC == ret) {
		FatalError("slave hot-backup not active yet");
	}

	//slave没有对应的node节点，拉取后调整lru
	if (-TTC::EC_KEY_NOTEXIST == ret) {
		if(log) {
			log_debug("hb adjust slave lru, fetch data from master");
		}

		return LoopUpdateSlaveData();
	}


	if (ret != 0) {
		if(log){
			log_warning("adjust slave lru error, msg: %s, from:%s",
					_lru_result.ErrorMessage(),
				       	_lru_result.ErrorFrom()
				 );
		}

		return -1;
	}

	return 0;
}

int CIncSyncValue::LoopAdjustSlaveLRU()
{
	int ret=0;
	int log=1;

	while ((ret=AdjustSlaveLRU(log))) {
		if(log) {
			log_warning("ajust slave lru error, enter loop retry"); 
		}

		if(getppid()==1) {
			break;
		}

		log=0;
		usleep(READER_SLEEP_TIME);
	}

	//停机的时候丢失一个key的lru调整请求，忽略
	if(ret)
	{
		log_warning("lost one adjusting slave lru request, ignore");
	}

	return 0;
}


int CIncSyncValue::PurgeDirtyKey(int log)
{
	TTC::SvrAdminRequest request(_slave);
	request.SetAdminCode(TTC::ReplaceRawData);

	request.EQ("key", _key.ptr, _key.len);
	request.Set("type", CHBGlobal::SYNC_PURGE);
	request.Set("flag", CHBGlobal::KEY_NOEXIST);

	int ret = request.Execute(_purge_result);

	if (ret != 0) {
		if(log) {
			log_warning("purge dirty key error, msg: %s, from:%s",
					_purge_result.ErrorMessage(),
					_purge_result.ErrorFrom()
				 );
		}

		return -1;
	}

	return 0;
}

int CIncSyncValue::LoopPurgeDirtyKey()
{
	int ret=0;
	int log=1;

	while (!DaemonBase::_stop && (ret=PurgeDirtyKey(log))) {
		if(log) {
			log_warning("purge dirty key error, enter loop retry");
		}

		log=0;
		usleep(READER_SLEEP_TIME);
	}

	//停机的时候丢失一个脏key的purge请求，
	if(ret) {
		FatalError("purge dirty key error");
	}

	return 0;
}

//当heartbeat检测到master异常后，切换到slave
//必须调用该接口purge掉hbp的日志。
int CIncSyncValue::PurgeAllLocalDirtyKeys()
{
	log_debug("hb process all local dirty keys");

	while (1) {

		_buff.clear();

		int ret = _logReader->Read(_buff);

		if (CHBGlobal::ASYNC_READER_WAIT_DATA == ret) {
			log_info("process all local dirty keys complete");
			break;
		}

		else if (CHBGlobal::ASYNC_PROCESS_OK != ret) {
			continue;
		}

		DECODE_FIELDS_FROM_BUFF(_buff);

		//如果没有附带value字段，则执行purge操作，如果有，则执行replace
		if(_flag == CHBGlobal::NON_VALUE) {
			if(PurgeDirtyKey(1)) {
				log_error("process all local dirty keys is not complete");
				return -1;
			}
		}else {
			if(ReplaceRawData(1)){
				log_error("process all local dirty keys is not complete");
				return -1;
			}
		}
	}

	return 0;
}

/**
 * 读取IncSyncKey存放的缓存数据，同步slave
 **/
int CIncSyncValue::Run()
{
	int sec = 0;
	struct timeval now;

	gettimeofday(&now, NULL);
	sec = now.tv_sec + 1;

	//不允许直接kill
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

		_buff.clear();

		int ret = _logReader->Read(_buff);

		if (ret == CHBGlobal::ASYNC_READER_WAIT_DATA) {
			usleep(READER_SLEEP_TIME);
			continue;

		} else if (ret != CHBGlobal::ASYNC_PROCESS_OK) {
			/* xxx help!!! */
			FatalError(_logReader->ErrorMessage());
		}

		DECODE_FIELDS_FROM_BUFF(_buff);

		switch (_type) {
		case CHBGlobal::SYNC_PURGE:
			LoopPurgeDirtyKey();
			break;

		case CHBGlobal::SYNC_DELETE:
		case CHBGlobal::SYNC_INSERT:
		case CHBGlobal::SYNC_UPDATE:
			LoopUpdateSlaveData();
			break;

		case CHBGlobal::SYNC_LRU:
			LoopAdjustSlaveLRU();
			break;

		default:
			log_error("unknown sync-type[%d], drop it", _type);
			break;
		}
	}
	_logReader->Commit();

	return 0;
}
