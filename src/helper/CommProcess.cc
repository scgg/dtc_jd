#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "CommProcess.h"
#include <protocol.h>
#include <dbconfig.h>
#include <log.h>
#include <task_base.h>

#include "proctitle.h"

#define CAST(type, var) type *var = (type *)addr

CCommHelper::CCommHelper():addr(NULL),check(0)
{
	Timeout = 0;
}

CCommHelper::~CCommHelper()
{
	addr = NULL;
}

CCommHelper::CCommHelper(CCommHelper& proc)
{

}

void CCommHelper::InitTitle(int group, int role) 
{
	snprintf(_name, sizeof(_name),
			"helper%d%c", 
			group, 
			MACHINEROLESTRING[role]
		);

	snprintf(_title, sizeof(_title),"%s: ", _name);
	_titlePrefixSize = strlen(_title);
	_title[sizeof(_title)-1] = '\0';
}               

void CCommHelper::SetTitle(const char *status)
{
	strncpy(_title + _titlePrefixSize, status, sizeof(_title)-1-_titlePrefixSize);
	set_proc_title(_title);
}

int CCommHelper::GobalInit(void)
{
	return 0;
}

int CCommHelper::Init(void)
{
	return 0;
}

int CCommHelper::Execute()
{
	int ret = -1;
	
	log_debug("request code: %d", RequestCode());
	
	switch(RequestCode()){
		/*  Nop/Purge/Flush这几个操作对于数据源来说不需要做操作（应该也不会有这样的请求） */
		case DRequest::Nop:
		case DRequest::Purge:
		case DRequest::Flush:
			return 0;

		case DRequest::Get:
			logapi.Start();
			ret = ProcessGet();
			logapi.Done(__FILE__, __LINE__, "SELECT", ret, ret);
			break;
			
		case DRequest::Insert:
			logapi.Start();
			ret = ProcessInsert();
			logapi.Done(__FILE__, __LINE__, "INSERT", ret, ret);
			break;
			
		case DRequest::Update:
			logapi.Start();
			ret = ProcessUpdate();
			logapi.Done(__FILE__, __LINE__, "UPDATE", ret, ret);
			break;
			
		case DRequest::Delete:
			logapi.Start();
			ret = ProcessDelete();
			logapi.Done(__FILE__, __LINE__, "DELETE", ret, ret);
			break;
			
		case DRequest::Replace:
			logapi.Start();
			ret = ProcessReplace();
			logapi.Done(__FILE__, __LINE__, "REPLACE", ret, ret);
			break;
			
		default:
			log_error("unknow request code");
			SetError(-EC_BAD_COMMAND, __FUNCTION__, "[Helper]invalid request-code");
			return -1;
	};
	
	return ret;
}

void CCommHelper::Attach(void* p)
{
	addr = p;
}

CTableDefinition* CCommHelper::Table(void) const
{
	CAST(CTask, task);
	return task->TableDefinition();
}

const CPacketHeader* CCommHelper::Header() const
{
#if 0
	CAST(CTask, task);
	return &(task->headerInfo);
#else
	// NO header anymore
	return NULL;
#endif
}

const CVersionInfo* CCommHelper::VersionInfo() const
{
	CAST(CTask, task);
	return &(task->versionInfo);
}

int CCommHelper::RequestCode() const
{
	CAST(CTask, task);
	return task->RequestCode();
}

int CCommHelper::HasRequestKey() const
{
	CAST(CTask, task);
	return task->HasRequestKey();
}

const CValue* CCommHelper::RequestKey() const
{
	CAST(CTask, task);
	return task->RequestKey();
}

unsigned int CCommHelper::IntKey() const
{
	CAST(CTask, task);
	return task->IntKey();
}

const CFieldValue* CCommHelper::RequestCondition() const
{
	CAST(CTask, task);
	return task->RequestCondition();
}

const CFieldValue* CCommHelper::RequestOperation() const
{
	CAST(CTask, task);
	return task->RequestOperation();
}

const CFieldSet* CCommHelper::RequestFields() const
{
	CAST(CTask, task);
	return task->RequestFields();
}

void CCommHelper::SetError(int err, const char *from, const char *msg)
{
	CAST(CTask, task);
	task->SetError(err, from, msg);
}

int CCommHelper::CountOnly(void) const
{
	CAST(CTask, task);
	return task->CountOnly();
}

int CCommHelper::AllRows(void) const
{
	CAST(CTask, task);
	return task->AllRows();
}

int CCommHelper::UpdateRow(CRowValue &row)
{
	CAST(CTask, task);
	return task->UpdateRow(row);
}

int CCommHelper::CompareRow(const CRowValue &row, int iCmpFirstNField) const
{
	CAST(CTask, task);
	return task->CompareRow(row, iCmpFirstNField);
}

int CCommHelper::PrepareResult(void)
{
	CAST(CTask, task);
	return task->PrepareResult();
}

void CCommHelper::UpdateKey(CRowValue *r)
{
	CAST(CTask, task);
	task->UpdateKey(r);
}

void CCommHelper::UpdateKey(CRowValue &r)
{
    UpdateKey (&r);
}

int CCommHelper::SetTotalRows(unsigned int nr)
{
	CAST(CTask, task);
	return task->SetTotalRows(nr);
}

int CCommHelper::SetAffectedRows(unsigned int nr)
{
	CAST(CTask, task);
	task->resultInfo.SetAffectedRows(nr);
	return 0;
}

int CCommHelper::AppendRow(const CRowValue &r)
{
	return AppendRow(&r);
}

int CCommHelper::AppendRow(const CRowValue *r)
{
	CAST(CTask, task);
	int ret = task->AppendRow(r);
	if(ret > 0) ret = 0;
	return ret;
}


/* 查询服务器地址 */
const char *CCommHelper::GetServerAddress(void) const
{
	return _server_string;
}

/* 查询配置文件 */
int CCommHelper::GetIntVal(const char *sec, const char *key, int def)
{
	return _config->GetIntVal(sec,key,def);
}

unsigned long long CCommHelper::GetSizeVal(const char *sec, const char *key, unsigned long long def, char unit)
{
	return _config->GetSizeVal(sec, key, def, unit);
}

int CCommHelper::GetIdxVal(const char *v1, const char *v2, const char *const *v3, int v4)
{
	return _config->GetIdxVal(v1, v2, v3, v4);
}

const char *CCommHelper::GetStrVal (const char *sec, const char *key)
{
	return _config->GetStrVal(sec, key);
}

bool CCommHelper::HasSection(const char *sec)
{
	return _config->HasSection(sec);
}

bool CCommHelper::HasKey(const char *sec, const char *key)
{
	return _config->HasKey(sec, key);
}

/* 查询表定义 */
int CCommHelper::FieldType(int n) const { return _tdef->FieldType(n); }
int CCommHelper::FieldSize(int n) const { return _tdef->FieldSize(n); }
int CCommHelper::FieldId(const char *n) const { return _tdef->FieldId(n); }
const char* CCommHelper::FieldName(int id) const { return _tdef->FieldName(id); }
int CCommHelper::NumFields(void) const { return _tdef->NumFields(); }
int CCommHelper::KeyFields(void) const { return _tdef->KeyFields(); }

