#include <stdio.h>
#include <stdlib.h>

#include "memcheck.h"
#include "multi_request.h"
#include "task_request.h"
#include "task_pkey.h"
#include "poll_thread.h"
#include "log.h"
#include "compiler.h"
#include "keylist.h"
#include "agent_multi_request.h"
#include "agent_client.h"

CTaskRequest::~CTaskRequest() 
{ 
        FreePackedKey(); 

        if(agentMultiReq)
	{
                delete agentMultiReq;  
		agentMultiReq = NULL;
	}

	if(recvBuff)
	{
		free(recvBuff);
		recvBuff = NULL;
	}
}

void CTaskRequest::Clean()
{
    blacklist_size = 0;
    timestamp = 0;
    barHash = 0;
    expire_time = 0;
    keyList = NULL;
    batchKey = NULL;
    CTask::Clean();
   // CTaskReplyList<CTaskRequest, 10>::Clean();
    CTaskOwnerInfo::Clean();
}

int CTaskRequest::BuildPackedKey(void) {
	
	if(unlikely(KeyFormat()==0))
		return BuildSingleStringKey();
	if(likely(KeyFields()==1))
		return BuildSingleIntKey();
	else
		return BuildMultiIntKey();
}

int CTaskRequest::BuildSingleStringKey(void) {
	CValue empty("");
	const CValue *key = RequestKey();
	if(key==NULL) key = &empty;

	if(key->str.len > FieldSize(0)) {
		SetError(-EC_KEY_NEEDED,"key verify","string key size overflow");
		return -1;
	}

	FREE_IF(packedKey);
	packedKey = (char *)MALLOC(key->str.len+1);
	if(packedKey==NULL) {
		SetError(-EC_SERVER_ERROR,"key verify","no enough memory");
		return -1;
	}

	int i;
	if(FieldType(0) == DField::String) {
		*packedKey = key->str.len;
		for(i=0; i<key->str.len; i++)
			packedKey[i+1] = INTERNAL_TO_LOWER(key->str.ptr[i]);
		log_debug("key type[string]");
	} else {
		*packedKey = key->str.len;
		memcpy(packedKey+1, key->str.ptr, key->str.len);
		log_debug("key type[binary]");
	}
	return 0;
}

int CTaskRequest::BuildSingleIntKey(void) {
	if(unlikely(KeyFormat() > 8)) {
		FREE_IF(packedKey);
		packedKey = (char *)calloc(1, KeyFormat());
		if(packedKey==NULL) {
			SetError(-EC_SERVER_ERROR,"key verify","no enough memory");
			return -1;
		}
	} else {
		packedKey = packedKeyBuf;
		memset(packedKey, 0, KeyFormat());
	}

	const CValue *key = RequestKey();
	if(likely(key != NULL)) {
		if(unlikely(CTaskPackedKey::StoreValue(packedKey, key->u64, FieldType(0), FieldSize(0)) < 0))
		{
			SetError(-EC_KEY_NEEDED,"key verify","int key value overflow");
			return -1;
		}
	} else if(!(KeyAutoIncrement() && RequestCode() == DRequest::Insert))
	{
		SetError(-EC_KEY_NEEDED,"key verify","No Key Sepcified");
		return -1;
	}
	return 0;
}

int CTaskRequest::BuildMultiKeyValues() {
	uint32_t keymask = 0;
	FREE_IF(multiKey);
	multiKey = (CValue *)calloc(sizeof(CValue), KeyFields());
	if(multiKey==NULL) {
		SetError(-ENOMEM,"key verify","NO ENOUGH MEMORY");
		return -1;
	}

	if(key != NULL) {
		multiKey[0] = *key;
		keymask |= 1;
	}

	const CFieldValue *kop = RequestCode()==DRequest::Insert || RequestCode()==DRequest::Replace ? updateInfo : conditionInfo;

	if (kop == NULL)
	{
			SetError(-EC_KEY_NEEDED,"key verify","Missing Key Component");
			return -1;
	}
	for(int i=0; i<kop->NumFields(); i++) {
		const int id = kop->FieldId(i);
		/* NOT KEY */
		if(id >= KeyFields())
			continue;

		if((keymask & (1<<id)) != 0) {
			SetError(-EC_BAD_MULTIKEY,"key verify","duplicate key field");
			return -1;
		}

		keymask |= 1<<id;
		const int vt = kop->FieldType(i);
		CValue *val = kop->FieldValue(i);
		// the operation must be EQ/Set, already checked in task_base.cc
		if(FieldSize(id) < 8 && val->s64 < 0 &&
		    ((FieldType(id)==DField::Unsigned && vt==DField::Signed) ||
		    (FieldType(id)==DField::Signed && vt==DField::Unsigned)))
		{
			SetError(-EC_KEY_OVERFLOW,"key verify","int key value overflow");
			return -1;
		}
		multiKey[id] = *val;
	}

	const int ai = AutoIncrementFieldId();
	if(ai >= 0 && ai < KeyFields() && RequestCode() == DRequest::Insert)
		keymask |= 1U<<ai;
	//keymask |= 1U;

	if((keymask+1) != (1U<<KeyFields())) {
		SetError(-EC_KEY_NEEDED,"key verify","Missing Key Component");
		return -1;
	}
	return 0;
}

int CTaskRequest::BuildMultiIntKey(void) {
	/* Build first key field */
	if(BuildSingleIntKey() < 0) return -1;

	if(FlagMultiKeyVal()==0) {
		// NOTICE, in batch request, BuildPackedKey only called by SetBatchCursor
		// which already set the corresponding multiKey member
		if(BuildMultiKeyValues() < 0) return -1;
	}
	
	for(int i = 1; i < KeyFields(); i++) {
		if(CTaskPackedKey::StoreValue(packedKey + FieldOffset(i), multiKey[i].u64,
					FieldType(i), FieldSize(i)) < 0)
		{
			SetError(-EC_KEY_OVERFLOW,"key verify","int key value overflow");
			return -1;
		}
	}
	return 0;
}

int CTaskRequest::UpdatePackedKey(uint64_t val) {
	int id = AutoIncrementFieldId();
	if(id >= 0 && id < KeyFields()) {
		if(multiKey) multiKey[id].u64 = val;
		if(CTaskPackedKey::StoreValue(packedKey + FieldOffset(id), val, FieldType(id), FieldSize(id)) < 0)
		{
			SetError(-EC_KEY_NEEDED,"key verify","int key value overflow");
			return -1;
		}
	}
	return 0;
}

void CTaskRequest::CalculateBarrierKey(void)
{
	if(packedKey) {
		barHash = CTaskPackedKey::CalculateBarrierKey(TableDefinition(), packedKey);
	}
}

int CTaskRequest::GetBatchSize(void) const {
       return batchKey != NULL ? batchKey->TotalCount() : 0;
}

void CTaskRequest::DumpUpdateInfo(const char *prefix) const
{
    /* for debug */
    return ;

    char buff[1024];
    buff[0] = 0;
    char *s = buff;

    const CFieldValue *p = RequestOperation();
    if(!p)
	return;

    for(int i = 0; i < p->NumFields(); i++)
    {
	//only dump setbits operations
	if(p->FieldOperation(i) == DField::SetBits)
	{
	    uint64_t f_v = p->FieldValue(i)->u64;

	    const int len = 8;
	    unsigned int off  = f_v >> 32;
	    unsigned int size = off >> 24;
	    off &= 0xFFFFFF; 
	    unsigned int value = f_v & 0xFFFFFFFF;


	    if(off >= 8*len || size == 0) break;
	    if(size > 32) size = 32;
	    if(size > 8*len-off)
		size = 8*len - off;

	    s += snprintf(s, sizeof(buff)-(s-buff), " [%d %d]", off, value);
	}
    }

    if(s != buff) 
	log_debug("[%s]%u%s", prefix, IntKey(), buff);

    return ;
}

#define ERR_RET(ret_msg, fmt, args...) do{ SetError(err, "decoder", ret_msg); log_debug(fmt, ##args); return -1; }while(0)
int CTaskRequest::PrepareProcess(void) {
	int err;

	if(1) {
		/* timeout present */
		int client_timeout = requestInfo.TagPresent(1) == 0 ?  DefaultExpireTime() :
			requestInfo.ExpireTime(versionInfo.CTLibIntVer());

		log_debug("client api set timeout %d ms", client_timeout);
		struct timeval now;
		gettimeofday(&now, NULL);

		responseTimer = (int)(now.tv_sec * 1000000ULL + now.tv_usec);
		expire_time = now.tv_sec * 1000ULL + now.tv_usec/1000 + client_timeout;
		timestamp = now.tv_sec;
	}


	if(0) {
#if 0
		// internal API didn't call PreparePrcess() !!!
		// move checker to task_api.cc
#endif
	} else if(FlagMultiKeyVal()) {
		// batch key present
		if(!versionInfo.TagPresent(10) || !versionInfo.TagPresent(11) || !versionInfo.TagPresent(12) || !versionInfo.TagPresent(13)){
			err = -EC_KEY_NEEDED;
			ERR_RET("require key field info", "require key field info: %d", err);
		}
		if((int)(versionInfo.GetTag(10)->u64) != KeyFields()){
			err = -EC_KEY_NEEDED;
			ERR_RET("key field count incorrect", "key field count incorrect. request: %d, table: %d", (int)(versionInfo.GetTag(10)->u64), KeyFields());
		}
		if(versionInfo.GetTag(11)->u64 < 1){
			err = -EC_KEY_NEEDED;
			ERR_RET("require key value", "require key value");
		}
		CArray keyTypeList(versionInfo.GetTag(12)->bin);
		CArray keyNameList(versionInfo.GetTag(13)->bin);
		uint8_t keyType;
		CBinary keyName;
		for(unsigned int i=0; i<versionInfo.GetTag(10)->u64; i++){
			if(keyTypeList.Get(keyType) != 0 || keyNameList.Get(keyName) != 0){
				err = -EC_KEY_NEEDED;
				ERR_RET("require key field info", "require key field info: %d", err);
			}
			int fid = FieldId(keyName.ptr);
			if(fid < 0 || fid >= KeyFields()){
				err = -EC_BAD_FIELD_NAME;
				ERR_RET("bad key field name", "bad key field name: %s", keyName.ptr);
			}
			int tabKeyType = FieldType(fid);
			if(keyType >= DField::TotalType || !validktype[keyType][tabKeyType]){
				err = -EC_BAD_KEY_TYPE;
				ERR_RET("key type incorrect", "key[%s] type[%d] != table-def[%d]", keyName.ptr, keyType, FieldType(FieldId(keyName.ptr)));
			}
		}
	} else if(RequestCode() == DRequest::SvrAdmin){
			// admin requests
			// Migrate命令需要在barrier里排队，所以需要校验是否有key以及计算hash
			if (DRequest::ServerAdminCmd::Migrate == requestInfo.AdminCode())
			{
					const CFieldValue* condition = RequestCondition();
					if (condition == 0||condition->NumFields()<=0||condition->FieldValue(0) == 0)
					{
							err = -EC_KEY_NEEDED;
							ERR_RET("migrate commond", "need set migrate key");
					}
					else if (condition->NumFields() > 1)
					{
							err = -EC_SERVER_ERROR;
							ERR_RET("migrate commond", "not support bulk migrate now");
					}
					const CValue* key = condition->FieldValue(0);

					if(key->str.len > 8) {
						if(packedKey!=packedKeyBuf && packedKey!=0)
						{
							free(packedKey);
							packedKey = NULL;
						}
						packedKey = (char *)calloc(1, key->str.len);
						if(packedKey==NULL) {
							SetError(-EC_SERVER_ERROR,"key verify","no enough memory");
							return -1;
						}
					} else {
						packedKey = packedKeyBuf;
						memset(packedKey, 0, KeyFormat());
					}

					if(packedKey==NULL) {
							SetError(-EC_SERVER_ERROR,"key verify","no enough memory");
							return -1;
					}
					memcpy(packedKey, key->bin.ptr, key->bin.len);

					CalculateBarrierKey();
			}

	} else {
			// single key request
		err = BuildPackedKey();
		if(err < 0) return err;

		CalculateBarrierKey();
	}

	return 0;
}



/* for agent request */
int CTaskRequest::DecodeAgentRequest()
{
    if(agentMultiReq)
    {
        delete agentMultiReq;
	agentMultiReq = NULL;
    }

    agentMultiReq = new CAgentMultiRequest(this);
    if(agentMultiReq == NULL)
    {
        log_crit("no mem new CAgentMultiRequest");
        return -1;
    }

    PassRecvedResultToAgentMutiReq();

    if(agentMultiReq->DecodeAgentRequest() < 0)
    {
       log_error("agent multi request decode error");
       return -1;
    }

    return 0;
}

void CTaskRequest::PassRecvedResultToAgentMutiReq()
{
	agentMultiReq->SaveRecvedResult(recvBuff, recvLen, recvPacketCnt);
	recvBuff = NULL; recvLen = recvPacketCnt = 0;
}

bool CTaskRequest::IsAgentRequestCompleted()
{
    return agentMultiReq->IsCompleted();
}

void CTaskRequest::DoneOneAgentSubRequest()
{
    CAgentMultiRequest * ownerReq = OwnerInfo<CAgentMultiRequest>();
    if(ownerReq)
        ownerReq->CompleteTask(OwnerIndex());
    else
        delete this;
}

void CTaskRequest::LinkToOwnerClient(CListObject<CAgentMultiRequest> & head)
{
	if(ownerClient && agentMultiReq)
		agentMultiReq->ListAdd(head);
}

void CTaskRequest::SetOwnerClient(CClientAgent * client)
{ 
	ownerClient = client; 
}

CClientAgent * CTaskRequest::OwnerClient()
{ 
	return ownerClient; 
}

void CTaskRequest::ClearOwnerClient()
{ 
	ownerClient = NULL; 
}

int CTaskRequest::AgentSubTaskCount()
{
    return agentMultiReq->PacketCount();
}

bool CTaskRequest::IsCurrSubTaskProcessed(int index)
{
    return agentMultiReq->IsCurrTaskProcessed(index);
}

CTaskRequest * CTaskRequest::CurrAgentSubTask(int index)
{
    return agentMultiReq->CurrTask(index);
}

void CTaskRequest::CopyReplyForAgentSubTask()
{
	agentMultiReq->CopyReplyForSubTask();
}

