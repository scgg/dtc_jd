#include <errno.h>
#include "comm.h"
#include "daemon.h"
#include "incsync_value.h"

#ifndef likely
#if __GCC_MAJOR >= 3
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif
#endif

#define GET_VALUE(x, t)  do {               \
        if(unlikely(in_uiOffset + sizeof(t) > all_size))    \
        goto ERROR_RET;         \
        x = (typeof(x))*(t *)(in_pchContent + in_uiOffset);\
        in_uiOffset += sizeof(t);       \
} while(0)

#define SKIP_SIZE(s)    do {            \
    if(unlikely(in_uiOffset + s > all_size))    \
    goto ERROR_RET;     \
    in_uiOffset += s;       \
} while(0)

int CIncSyncValue::SkipRow(CRowValue &stRow, char * in_pchContent,int & in_uiOffset,unsigned int all_size, int in_uchKeyIdx)
{
    in_uiOffset += sizeof(unsigned char);

    for (int j = in_uchKeyIdx + 1; j <= stRow.NumFields() ; j++)//拷贝一行数据
    {
        //id: bug fix skip discard
        if(stRow.TableDefinition()->IsDiscard(j)) continue;

        switch(stRow.FieldType(j))
        {
        case DField::Unsigned:
        case DField::Signed:
            if(stRow.FieldSize(j) > (int)sizeof(int32_t) )
                SKIP_SIZE(sizeof(int64_t));
            else
                SKIP_SIZE(sizeof(int32_t));;
            break;

        case DField::Float:         //浮点数
            if(stRow.FieldSize(j) > (int)sizeof(float) )
                SKIP_SIZE(sizeof(double));
            else
                SKIP_SIZE(sizeof(float));
            break;

        case DField::String:        //字符串
        case DField::Binary:        //二进制数据
        default:
            {
                unsigned int iLen;
                GET_VALUE(iLen, int);
                SKIP_SIZE(iLen);
                break;
            }
        }//end of switch
    }
    return(0);

ERROR_RET:
    return(-100);
}

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
    

    dbConfig = CDbConfig::Load(CComm::tablename);
    if(dbConfig == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"load table configuire file error.\n");
        return -2;
    }

    gTableDef = dbConfig->BuildTableDefinition();
    if(gTableDef == NULL)
    {
        snprintf(_errmsg,sizeof(_errmsg),"build table definition error.\n");
        return -3;
    }

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

    if(gTableDef->LastcmodFieldId() > 0 || _flag == CHBGlobal::KEY_NOEXIST) //如果主机配置了lastcmod字段，则直接使用主机的数据热备
    {
        TTC::SvrAdminRequest request(_slave);
        request.SetAdminCode(TTC::ReplaceRawData);

        request.EQ("key", _key.ptr, _key.len);
        request.Set("type", _type);
        request.Set("flag", _flag);
        request.Set("value", _value.ptr, _value.len);

        int ret = request.Execute(_replace_result);
        if (-EBADRQC == ret) 
        {   
            FatalError("slave hot-backup not active yet");
        }   

        if (ret != 0)  
        {   
            if(log) 
            {   
                log_warning("replace error, msg: %s, from:%s",
                        _replace_result.ErrorMessage(),
                        _replace_result.ErrorFrom());
            }   

            return -1; 
        }   

        return 0;

    }

    CBinary _newvalue;
    CRowValue* pstRow;
    pstRow = new CRowValue(gTableDef);
    if(pstRow == NULL)
    {
        fprintf(stderr, "new row error: %m\n");
        return -1;
    }

    //根据老ttc的value，计算新ttc的value信息；
    //新ttc value是在老ttc value的每行最后添加一个timestamp字段
    //timestamp字段固定为4个字节，因为time（）的返回值类型是time_t
    //time_t的大小为4个字节
    CRawFormat rawdata  = *(CRawFormat *)_value.ptr;

    int offset = 9 + _key.len;
    //key有rownum行数据
    unsigned int rownum = rawdata.m_uiRowCnt;
    //新数据的大小
    int newvalue_len = _value.len + sizeof(int) * (rownum);
    
    _newvalue.len = newvalue_len;
    _newvalue.ptr = (char *)malloc(sizeof(char)*newvalue_len);
    
    memcpy(_newvalue.ptr,(const void*)_value.ptr,offset);
    *(int *)(_newvalue.ptr + sizeof(unsigned char)) += sizeof(int) * rownum; 
    //重新设置新ttc 数据 的 m_uiDataSize
    
    time_t timestamp = time((time_t *)NULL);
    if(timestamp == -1)
    {
        if(log) 
        {
            log_warning("get time failed");
        }
        return -1;
    }

    //copy老ttc的每行数据给新ttc，并在新ttc数据后面加上时间戳
    
    int rowlen = 0;
    int iret = 0;
    int begin_offset = 0;
    int new_offset = offset;
    
    
    for(unsigned int i = 0; i< rownum; i++)
    {
        //计算一行的长度
        begin_offset = offset;
        iret =  SkipRow(*pstRow,_value.ptr,offset,rawdata.m_uiDataSize,gTableDef->KeyFields()-1);
        if(iret != 0 )
        {
            if(log)
            {
                log_warning("get row size failed");
            }
            return -1;
        }
        rowlen = offset - begin_offset;
        
        //将老ttc的数据拷贝给新ttc的value
        memcpy(_newvalue.ptr + new_offset,_value.ptr + begin_offset,rowlen);
        new_offset += rowlen;
        //将时间戳写入老数据；
        memcpy(_newvalue.ptr + new_offset,(const void *)&timestamp,sizeof(int));
        new_offset += sizeof(int);
    }

    TTC::SvrAdminRequest request(_slave);
    request.SetAdminCode(TTC::ReplaceRawData);

    request.EQ("key", _key.ptr, _key.len);
    request.Set("type", _type);
    request.Set("flag", _flag);
    request.Set("value", _newvalue.ptr, _newvalue.len);

    int ret = request.Execute(_replace_result);
    
    delete pstRow;
    free(_newvalue.ptr);

    if (-EBADRQC == ret) 
    {
        FatalError("slave hot-backup not active yet");
    }

    if (ret != 0) 
    {
        if(log) 
        {
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
			//LoopAdjustSlaveLRU();
			break;

		default:
			log_error("unknown sync-type[%d], drop it", _type);
			break;
		}
	}
	_logReader->Commit();

	return 0;
}
