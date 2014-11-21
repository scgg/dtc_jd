/* vim: set sw=4 ai: */
#include <unistd.h>
#include <errno.h>
#include <endian.h>
#include <byteswap.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include "version.h"

#include "decode.h"
#include "protocol.h"
#include "cache_error.h"
#include "log.h"
#include "task_request.h"
#include "result.h"
#include <ttcint.h>

inline int CFieldSet::Copy(const CFieldSetByName &rq)
{
    	const int num = rq.NumFields();
	for(int n=0; n<num; n++) {
	    const int id = rq.FieldId(n);
	    // filter out invalid and duplicated fieldId
	    if(id>=0 && id<INVALID_FIELD_ID && !FieldPresent(id))
		AddField(id);
	}
	return 0;
}

int CFieldValue::Copy(const CFieldValueByName &rq, int mode, const CTableDefinition *tdef)
{
    	int clear = 0;
	const int num = rq.NumFields();
	unsigned knum = (int)tdef->KeyFields();
	for(int n=0; n<num; n++) {
	    unsigned id = rq.FieldId(n);
	    const CValue *val = rq.FieldValue(n);
	    unsigned op = rq.FieldOperation(n);
	    unsigned vt = rq.FieldType(n);

	    const int ft = tdef->FieldType(id);

	    if(vt==0 || vt >= DField::TotalType){
		// value type is invalid
		return -EC_BAD_VALUE_TYPE;
	    }

	    if(mode==0) {
		// condition information
		if(op >= DField::TotalComparison || CTask::validcomps[ft][op]==0) {
		    // invalid comparator id or
		    // the field type don't support the comparator
		    return -EC_BAD_OPERATOR;
		}

		if(id < knum) {
		    // part of cache hash key fields
		    if(op != DField::EQ) {
			// key must always EQ
			return -EC_BAD_MULTIKEY;
		    }
		} else {
		    // non-key condition exists, clear AllRows flag
		    clear = 1;
		}

		if(CTask::validxtype[DField::Set][ft][vt]==0) {
		    // value type must compatible with field type
		    return -EC_BAD_OPERATOR;
		}
	    } else if(mode==1) {
		// mode 1 is insert/replace
	    	if(op != DField::Set) {
		    // insert values, must be assignment
		    return -EC_BAD_OPERATOR;
		}
		if(CTask::validxtype[op][ft][vt]==0) {
		    // value type must compatible with field type
		    return -EC_BAD_OPERATOR;
		}
	    } else {
		// update operation
		if(tdef->IsReadOnly(id)) {
		    // read-only field cannot be updated
		    return -EC_READONLY_FIELD;
		}
		if(op >= DField::TotalOperation || CTask::validxtype[op][ft][vt]==0) {
		    // invalid operator id or
		    // the field type don't support the operator
		    return -EC_BAD_OPERATOR;
		}
	    }

	    // everything is fine
	    AddValue(id, op, vt, *val); // val never bu NULL
	    // non key field, mark its type
	    UpdateTypeMask(tdef->FieldFlags(id));
	}

	return clear;
}

int CTaskRequest::Copy(NCRequest &rq, const CValue *kptr)
{
	if(1) {
		/* timeout present */
		int client_timeout = requestInfo.TagPresent(1) ?  requestInfo.ExpireTime(3) : DefaultExpireTime();

		//log_debug("client api set timeout %d ms", client_timeout);
		struct timeval now;
		gettimeofday(&now, NULL);

		responseTimer = (int)(now.tv_sec * 1000000ULL + now.tv_usec);
		expire_time = now.tv_sec * 1000ULL + now.tv_usec/1000 + client_timeout;
		timestamp = now.tv_sec;
	}


	// this Copy() may be failed
	int ret = CTask::Copy(rq, kptr);
	if(ret < 0) return ret;

	// inline from PrepareProcess()
	if((requestFlags & DRequest::Flag::MultiKeyValue)) {
	    if(rq.kvl.KeyFields() != KeyFields()) {
		SetError(-EC_KEY_NEEDED, "decoder", "key field count incorrect" );
		return -EC_KEY_NEEDED;
	    }
	    if(rq.kvl.KeyCount() < 1){
		SetError(-EC_KEY_NEEDED, "decoder", "require key value" );
		return -EC_KEY_NEEDED;
	    }
	    // set batch key
	    keyList = &rq.kvl;
	} else if(RequestCode() != DRequest::SvrAdmin) {
		if(BuildPackedKey() < 0)
			return -1; // ERROR
		CalculateBarrierKey();
	}

	return 0;
}

#define ERR_RET(ret_msg, fmt, args...) do { SetError(err, "decoder", ret_msg); log_debug(fmt, ##args); return err; } while(0)
int CTask::Copy(NCRequest &rq, const CValue *kptr)
{
	NCServer *sv = rq.server;

	CTableReference::SetTableDefinition(rq.tdef);
	stage = DecodeStageDone;
	role = TaskRoleServer;
	requestCode = rq.cmd;
	requestType = cmd2type[requestCode];

#define PASSING_FLAGS	( \
	DRequest::Flag::NoCache | \
	DRequest::Flag::NoResult | \
	DRequest::Flag::NoNextServer | \
	DRequest::Flag::MultiKeyValue | \
	0 )

	requestFlags = DRequest::Flag::KeepAlive | (rq.flags & PASSING_FLAGS);

	// tablename & hash
	versionInfo.SetTableName(rq.tablename);
	if(rq.tdef) versionInfo.SetTableHash(rq.tdef->TableHash());
	versionInfo.SetSerialNr(sv->NextSerialNr());
	// app version
	if(sv->appname) versionInfo.SetTag(5, sv->appname);
	// lib version
	versionInfo.SetTag(6, "ctlib-v"TTC_VERSION);
	versionInfo.SetTag(9, rq.keytype);
	
	// hot backup id
	versionInfo.SetHotBackupID(rq.hotBackupID);

	// hot backup timestamp
	versionInfo.SetMasterHBTimestamp(rq.MasterHBTimestamp);
	versionInfo.SetSlaveHBTimestamp(rq.SlaveHBTimestamp);
	if(sv->tdef && rq.adminCode != 0)
		versionInfo.SetDataTableHash(sv->tdef->TableHash());

	if(rq.flags & DRequest::Flag::MultiKeyValue) {
		kptr = NULL;
		if(sv->SimpleBatchKey() && rq.kvl.KeyCount()==1) {
			/* single field single key batch, convert to normal */
			kptr = rq.kvl.val;
			requestFlags &= ~DRequest::Flag::MultiKeyValue;
		}
	}

	key = kptr;
	processFlags = PFLAG_ALLROWS;

	if(kptr) {
	    requestInfo.SetKey(*kptr);
	}
	// cmd
	if(sv->GetTimeout()) {
	    requestInfo.SetTimeout(sv->GetTimeout());
	}
	//limit
	if(rq.limitCount) {
	    requestInfo.SetLimitStart(rq.limitStart);
	    requestInfo.SetLimitCount(rq.limitCount);
	}
	if(rq.adminCode > 0){
		requestInfo.SetAdminCode(rq.adminCode);
	}

	if(rq.fs.NumFields() > 0) {
	    fieldList = new CFieldSet(rq.fs.NumFields());
	    fieldList->Copy(rq.fs); // never failed
	}

	if(rq.ui.NumFields() > 0) {
	    /* decode updateInfo */
	    updateInfo = new CFieldValue(rq.ui.NumFields());
	    const int mode = requestCode==DRequest::Update ? 2 : 1;
	    int err = updateInfo->Copy(rq.ui, mode, rq.tdef);
	    if(err < 0) ERR_RET("decode update info error", "decode update info error: %d", err);
	}

	if(rq.ci.NumFields() > 0) {
	    /* decode conditionInfo */
	    conditionInfo = new CFieldValue(rq.ci.NumFields());
	    int err = conditionInfo->Copy(rq.ci, 0, rq.tdef);
	    if(err < 0) ERR_RET("decode condition info error", "decode update info error: %d", err);
	    if(err > 0) {
		ClearAllRows();
	    }
	}

	return 0;
}

int CTask::ProcessInternalResult(uint32_t ts)
{
	CResultPacket *rp = ResultCode() >= 0 ? GetResultPacket() : NULL;

	if(rp) {
	    resultInfo.SetTotalRows(rp->totalRows);
	} else {
	    resultInfo.SetTotalRows(0);
	    if(ResultCode()==0)
		 SetError(0, NULL, NULL);
	}

	if(ts) {
		resultInfo.SetTimestamp(ts);
	}
	versionInfo.SetTag(6, "ctlib-v"TTC_VERSION);
	if(ResultKey()==NULL && RequestKey()!=NULL)
		SetResultKey(*RequestKey());
	
	replyFlags = DRequest::Flag::KeepAlive|FlagMultiKeyVal();
	if(rp==NULL) {
	    replyCode = DRequest::ResultCode;
	} else {
	    replyCode = DRequest::ResultSet;
	    CBinary v;
	    v.ptr = rp->bc->data + rp->rowDataBegin;
	    v.len = rp->bc->usedBytes - rp->rowDataBegin;
	    if(rp->fieldSet)
		result = new CResultSet(*rp->fieldSet, TableDefinition());
	    else
		/* newman: pool */
		result = new CResultSet((const uint8_t *)"", 0, NumFields(), TableDefinition());
	    result->SetValueData(rp->numRows, v);
	}
	return 0;
}

