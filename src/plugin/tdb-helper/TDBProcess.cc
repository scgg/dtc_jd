#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <log.h>
#include "CommProcess.h"
#include "TDBConn.h"
#include "TDBProcess.h"
#include "tdb_error.h"
#include "cache_error.h"

// so库的接口函数。
extern "C" CCommHelper* create_process(void)
{
	return new CTDBHelper;
}

TTC_USING_NAMESPACE

CTDBHelper::CTDBHelper()
{
}

CTDBHelper::~CTDBHelper()
{
}

int CTDBHelper::GobalInit(void)
{
	/* build tdb configuration */
	const char *addr = GetServerAddress();
	if(addr==NULL || strncasecmp(addr, "TDB:", 4) != 0)
	{
		log_error("__NOT__ support tdb helper, gourpid=%d", GroupID);
		return -1;
	}

	const char *tdbsection = addr + strlen("TDB:");
	t_id = GetIntVal(tdbsection, "TID", 0);
	c_id = GetIntVal(tdbsection, "CID", 0);

	log_debug("t_id=%u, c_id=%u", t_id, c_id);
	if(t_id == 0 || c_id == 0)
	{
		log_error("[TDB]'s tid, cid argument is invalid");
		return -EINVAL;
	}

	timeout = GetIntVal(tdbsection, "Timeout", 5);
	freeze  = GetIntVal(tdbsection, "FreezeTime", 180);
	route = GetStrVal(tdbsection, "RouteAddr") ? : NULL;
	retryflushtime = GetIntVal(tdbsection, "RetryFlushTime", 3600);
	
	int count = GetIntVal(tdbsection, "Count", 0);
	if(count == 0)
	{
		log_error("not found TDB helper addrs");
		return -EINVAL;
	}

	for(int i = 0; i < count; ++i)
	{
		char fieldStr[256];
		snprintf(fieldStr, sizeof(fieldStr), "Addr%i", i+1);
		const char* addr = GetStrVal(tdbsection, fieldStr);
		if(addr == NULL || addr[0] == 0)
		{
			log_error("TDB %s not found", fieldStr);
			return -EINVAL;
		}
		hands.push_back(std::string(addr));
		if(i==0)
			logapi.InitTarget(addr);
	}
	return(0);
}

int CTDBHelper::Init(void)
{
	// init TDB connection
	if(_tdbConn->Open(this))
	{
		log_error("connect to tdb server failed, %d, %s", _tdbConn->ErrorNo(), _tdbConn->Error());
		return -1;
	}

	return 0;
}

int CTDBHelper::MakeKeyString(std::string& key)
{
	key.clear();
	/* field0 -> key */
	switch(FieldType(0))
	{
		case DField::Signed:
			key.assign((const char*)&(RequestKey()->s64), sizeof(uint64_t));
			break;

		case DField::Unsigned:
			key.assign((const char*)&(RequestKey()->u64), sizeof(uint64_t));
			break;

		case DField::Float:
			key.assign((const char*)&(RequestKey()->flt), sizeof(double));
			break;

		case DField::String:
		case DField::Binary:
			key.assign((const char*)(RequestKey()->str.ptr), RequestKey()->str.len
			 	  );
			break;
	}

	return 0;
}

/* TODO 这里可以做到解释TDB字段 */
/* 目前还没做，直接把value作为第一个field返回*/
int CTDBHelper::DecodeRow(CRowValue& row, std::string& value)
{
	for(int i=0, j=KeyFields(); i < NumFields();++i, ++j)
	{
		switch(FieldType(j))
		{
			case DField::String:
			case DField::Binary:
				row.FieldValue(j)->str.ptr = (char *)value.c_str();
				row.FieldValue(j)->str.len = value.size();
				break;
			case DField::Signed:
			case DField::Unsigned:
			case DField::Float:
			default:
				return -1;
				break;
		}
	}

	return 0;
}

/* build 需要set的内容 */
int CTDBHelper::MakeValueString(const CValue* fv, int ft, std::string& value)
{
	/* TDB 不支持设置空值 */
	if(!fv)
		return -1;

	switch(ft)
	{
		case DField::String:
		case DField::Binary:
			value.assign((const char *)(fv->bin.ptr), (int)(fv->bin.len));
			break;

		/*以后可以考虑支持其他类型的字段*/
		case DField::Signed:
		case DField::Unsigned:
		case DField::Float:
		default:
			return -1;
			break;
	}

	return 0;
}

int CTDBHelper::EncodeRow(std::string& value)
{
	const CFieldValue* setInfo = RequestOperation();
	if(!setInfo)
	{
		log_error("not found set conditions");
		return -1;
	}

	/* Set条件直接重组为TDB的value */
	for(int i = 0; i < setInfo->NumFields(); ++i)
	{
		switch(setInfo->FieldOperation(i))
		{
			case DField::Set:
				if(MakeValueString(setInfo->FieldValue(i), 
						   setInfo->FieldType(i),
						   value))
				{
					log_error("build value string failed");
					return -1;
				}
				break;
			default:
				log_warning("tdb-helper only support set operation");
				return -1;
				break;
		}
	}

	return 0;
}

int CTDBHelper::ProcessGet(void)
{
	std::string key;
	std::string value;

	if(MakeKeyString(key))
	{
		SetError(-EC_ERROR_BASE, __FUNCTION__, "make key string failed");
		log_error("build key string failed");
		return -1;
	}

	logapi.Start();
	int ret = _tdbConn->Get(key, value);
	logapi.Done(__FILE__, __LINE__, "SELECT", ret, _tdbConn->ErrorNo());

	if(ret == -E_TDB_NO_RECORD || ret == -E_TDB_NO_BUCKET || ret == -E_TDB_WCACHE_DEL) /*没有记录，不能返回错误*/
	{
		PrepareResult(); 
		return 0;
	}

	if(ret)
	{
		SetError(_tdbConn->ErrorNo(), __FUNCTION__, _tdbConn->Error());
		log_error("get data from tdb server failed, %d, %s", _tdbConn->ErrorNo(),_tdbConn->Error());
		return -1;
	}

	PrepareResult();

	/* TDB是单行记录 */
	CRowValue row(Table());
	if(DecodeRow(row, value))
	{
		SetError(-EC_ERROR_BASE, __FUNCTION__, "decode row failed");
		log_error("decode row failed");
		return -1;
	}
	UpdateKey(&row);
	AppendRow(&row);
	return 0;
}

int CTDBHelper::ProcessReplace(void)
{
	std::string key;
	std::string value;

	if(MakeKeyString(key))
	{
		SetError(-EC_ERROR_BASE, __FUNCTION__, "make key string failed");
		log_error("build key string failed");
		return -1;
	}

	if(EncodeRow(value))
	{
		SetError(-EC_ERROR_BASE, __FUNCTION__, "encode row failed");
		log_error("encode row failed");
		return -1;
	}

	int ret, wait = 1;
	for(unsigned i = 0; i <= retryflushtime; i +=wait)
	{
		logapi.Start();
		ret = _tdbConn->Set(key, value);
		logapi.Done(__FILE__, __LINE__, "SELECT", ret, _tdbConn->ErrorNo());
		if(ret)
		{
			wait = wait << 1;
			wait = wait < 64 ? wait:64;
			sleep(wait);
		}
		else
			break;
	}
	
	if(ret)
	{
		SetError(_tdbConn->ErrorNo(), __FUNCTION__, _tdbConn->Error());
		log_crit("FATAL: loop replace data failed, %d, %s", _tdbConn->ErrorNo(), _tdbConn->Error());
		return -1;
	}

	SetAffectedRows(_tdbConn->AffectedRows());
	return 0;
}

int CTDBHelper::ProcessDelete(void)
{
	if(AllRows() == 0)
	{
		SetError(-EC_BAD_COMMAND, __FUNCTION__, _tdbConn->Error());
		log_error("tdb delete command not support condition, %d, %s", _tdbConn->ErrorNo(), _tdbConn->Error());
		return -1;
	}
		
	std::string key;

	if(MakeKeyString(key))
	{
		SetError(-EC_ERROR_BASE, __FUNCTION__, "make key string failed");
		log_error("build key string failed");
		return -1;
	}

	logapi.Start();
	int ret = _tdbConn->Del(key);
	logapi.Done(__FILE__, __LINE__, "SELECT", ret, _tdbConn->ErrorNo());	
	if(ret)
	{
		SetError(_tdbConn->ErrorNo(), __FUNCTION__, _tdbConn->Error());
		log_error("tdb delete data failed, %d, %s", _tdbConn->ErrorNo(), _tdbConn->Error());
		return -1;
	}

	SetAffectedRows(_tdbConn->AffectedRows());
	return 0;
}

int CTDBHelper::ProcessInsert(void)
{
	SetError(-EC_BAD_COMMAND, __FUNCTION__, "invalid request command");
	return(-1);
}

int CTDBHelper::ProcessUpdate(void)
{
	SetError(-EC_BAD_COMMAND, __FUNCTION__, "invalid request command");
	return(-1);
}
