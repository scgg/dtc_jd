/* vim: set sw=4 ai: */
#include <unistd.h>
#include <errno.h>
#include <endian.h>
#include <byteswap.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include "decode.h"
#include "protocol.h"
#include "cache_error.h"
#include "log.h"
#include "task_request.h"
#include "task_pkey.h"


// all Copy() only copy raw request state,
// don't copy in-process data

// vanilla Copy(), un-used yet
int CTask::Copy(const CTask &rq)
{
    	//straight member-by-member copy or duplicate
	CTableReference::SetTableDefinition(rq.TableDefinition());
	stage = DecodeStageDone;
	role = TaskRoleServer;

	versionInfo.Copy(rq.versionInfo);
	requestInfo.Copy(rq.requestInfo);
	resultInfo.Copy(rq.resultInfo);

	key = rq.RequestKey();
	requestCode = rq.requestCode;
	requestType = rq.requestType;
	requestFlags = rq.requestFlags;
	processFlags = rq.processFlags &(PFLAG_ALLROWS|PFLAG_FIELDSETWITHKEY|PFLAG_PASSTHRU);
	if(TableDefinition()) TableDefinition()->INC();
	fieldList = rq.fieldList ? new CFieldSet(*rq.fieldList) : NULL;
	updateInfo = rq.updateInfo ? new CFieldValue(*rq.updateInfo) : NULL;
	conditionInfo = rq.conditionInfo ? new CFieldValue(*rq.conditionInfo) : NULL;

	return 0;
}

// vanilla Copy(), un-used yet
int CTaskRequest::Copy(const CTaskRequest &rq)
{
	CTask::Copy(rq); // always success
	expire_time = rq.expire_time;
	timestamp = rq.timestamp;

	// don't use generic BuildPackedKey, copy manually
	barHash = rq.barHash;
	// Dup multi-fields key
	if(rq.multiKey) {
		multiKey = (CValue *)MALLOC(KeyFields()*sizeof(CValue));
		if(multiKey==NULL) throw -ENOMEM;
		memcpy(multiKey, rq.multiKey, KeyFields()*sizeof(CValue));
	}
	// Dup packed key
	if(rq.packedKey) {
		int pksz = CTaskPackedKey::PackedKeySize(packedKey, KeyFormat());
		packedKey = (char *)MALLOC(pksz);
		if(packedKey==NULL) throw -ENOMEM;
		memcpy(packedKey, rq.packedKey, pksz);
	}
	// Copy internal API batch key list
	keyList = rq.InternalKeyValList();
	return 0;
}

// only for batch request spliting
int CTask::Copy(const CTask &rq, const CValue *newkey)
{
    	// copy non-field depend informations, see above Copy() variant
	CTableReference::SetTableDefinition(rq.TableDefinition());
	stage = DecodeStageDone;
	role = TaskRoleServer;
	versionInfo.Copy(rq.versionInfo);
	//如果是批量请求，拷贝批量task的versioninfo之后记得强制set一下keytype
	//因为有些老的api发过来的请求没有设置全局的keytype
	//这样批量的时候可能会在heper端出现-2024错误
	if (((CTaskRequest*)&rq)->IsBatchRequest())
	{
		if (TableDefinition())
			versionInfo.SetKeyType(TableDefinition()->KeyType());
		else
			log_notice("TableDefinition() is null.please check it");
	}
	requestInfo.Copy(rq.requestInfo);
	resultInfo.Copy(rq.resultInfo);
	key = newkey;
	requestCode = rq.requestCode;
	requestType = rq.requestType;
	requestFlags = rq.requestFlags & ~DRequest::Flag::MultiKeyValue;
	processFlags = rq.processFlags &(PFLAG_ALLROWS|PFLAG_FIELDSETWITHKEY|PFLAG_PASSTHRU);
	if(TableDefinition()) TableDefinition()->INC();

	// Need() field list always same
	fieldList = rq.fieldList ? new CFieldSet(*rq.fieldList) : NULL;

	// primary key set to first key component
	requestInfo.SetKey(newkey[0]);

	// k1 means key field number minus 1
	const int k1 = rq.KeyFields() - 1;
	if(k1 <= 0)
	{
	    // single-field key, copy straight
	    updateInfo = rq.updateInfo ? new CFieldValue(*rq.updateInfo) : NULL;
	    conditionInfo = rq.conditionInfo ? new CFieldValue(*rq.conditionInfo) : NULL;
	}
	else if(rq.requestCode==DRequest::Insert || rq.requestCode==DRequest::Replace)
	{
	    // multi-field key, insert or replace
	    conditionInfo = rq.conditionInfo ? new CFieldValue(*rq.conditionInfo) : NULL;

	    // enlarge update information, add non-primary key using Set() operator
	    updateInfo = rq.updateInfo ? new CFieldValue(*rq.updateInfo, k1) : new CFieldValue(k1);
	    for(int i=1; i<=k1; i++){ // last one is k1
		updateInfo->AddValue(i, DField::Set, /*FieldType(i)*/DField::Signed, newkey[i]);
		// FIXME: server didn't support non-integer multi-field key
	    }

	} else {
	    // multi-field key, other commands
	    updateInfo = rq.updateInfo ? new CFieldValue(*rq.updateInfo) : NULL;

	    // enlarge condition information, add non-primary key using EQ() comparator
	    conditionInfo = rq.conditionInfo ? new CFieldValue(*rq.conditionInfo, k1) : new CFieldValue(k1);
	    for(int i=1; i<=k1; i++){ // last one is k1
		conditionInfo->AddValue(i, DField::EQ, /*FieldType(i)*/DField::Signed, newkey[i]);
		// FIXME: server didn't support non-integer multi-field key
	    }
	}

	return 0;
}

int CTaskRequest::Copy(const CTaskRequest &rq, const CValue *newkey)
{
	CTask::Copy(rq, newkey); // always success
	expire_time = rq.expire_time;
	timestamp = rq.timestamp;
	// splited, now NOT batch request
	// re-calculate packed and barrier key
	if(RequestCode() != DRequest::SvrAdmin) {
		if(BuildPackedKey() < 0)
			return -1; // ERROR
		CalculateBarrierKey();
	}
	return 0;
}

// only for commit row, replace whole row
int CTask::Copy(const CRowValue &r) {
	CTableReference::SetTableDefinition(r.TableDefinition());
	stage = DecodeStageDone;
	role = TaskRoleServer;
	requestCode = DRequest::Replace;
	requestType = cmd2type[requestCode];
	requestFlags = DRequest::Flag::KeepAlive;
	processFlags = PFLAG_ALLROWS|PFLAG_FIELDSETWITHKEY;

	const int n = r.NumFields();

	// dup strings&binary field value to packetbuf
	CValue row[n+1];
	int len = 0;
	for(int i=0; i<=n; i++) {
	    int t = r.FieldType(i);
	    if(t == DField::String || t == DField::Binary)
		len += r[i].bin.len+1;
	}
	char *p = packetbuf.Allocate(len, role);
	for(int i=0; i<=n; i++) {
	    int t = r.FieldType(i); 
	    if(t == DField::String || t == DField::Binary) {
		int l = r[i].bin.len;
		row[i].bin.len = l;
		row[i].bin.ptr = p;
		memcpy(p, r[i].bin.ptr, l);
		p[l] = 0;
		p += l+1;
	    } else {
		row[i] = r[i];
	    }
	}

	// tablename & hash
	versionInfo.SetTableName(TableName());
	versionInfo.SetTableHash(TableHash());
	versionInfo.SetSerialNr(0);
	versionInfo.SetTag(9, KeyType());

	// key
	requestInfo.SetKey(row[0]);
	// cmd
	requestInfo.SetTag(1, DRequest::Replace);

	// a replace command with all fields set to desired value
	updateInfo = new CFieldValue(n);
	for(int i=1; i <= n; i++)
	    updateInfo->AddValue(i, DField::Set, 
				//api cast all uint to long long,and set fieldtype to uint
				//so here should be consistent
				(r.FieldType(i)==DField::Unsigned)?DField::Signed:r.FieldType(i),
				row[i]);

	// first field always is key
	// bug fix, key should not point to local row[0]
	key = requestInfo.Key();;

	return 0;
}

int CTaskRequest::Copy(const CRowValue &row)
{
	CTask::Copy(row); // always success

	// ALWAYS a replace request
	// calculate packed and barrier key
	if(BuildPackedKey() < 0)
		return -1; // ERROR
	CalculateBarrierKey();
	return 0;
}

