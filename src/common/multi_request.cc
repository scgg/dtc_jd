#include "multi_request.h"
#include "task_request.h"
#include "task_multiplexer.h"
#include "keylist.h"
#include "memcheck.h"

static CMultiTaskReply multiTaskReply;

CMultiRequest::CMultiRequest(CTaskMultiplexer *o, CTaskRequest* task) :
	owner(o),
	wait(task),
	keyList(NULL),
	keyMask(NULL),
	doneReq(0),
	totalReq(0),
	subReq(0),
	firstPass(1),
	keyFields(0),
	internal(0)
{
}

CMultiRequest::~CMultiRequest()
{
	if(wait) {
		wait->ReplyNotify();
		wait = NULL;
	}
	if(internal==0)
		FREE_IF(keyList);
	FREE_IF(keyMask);
}

void CMultiTaskReply::ReplyNotify(CTaskRequest *cur)
{
	CMultiRequest *req = cur->OwnerInfo<CMultiRequest>();
	if(req==NULL)
		delete cur;
	else
		req->CompleteTask(cur, cur->OwnerIndex());
}

CValue *CMultiRequest::GetKeyValue(int i) {
	return &keyList[i*keyFields];
}

void CMultiRequest::SetKeyCompleted(int i) {
	FD_SET(i, (fd_set *)keyMask);
	doneReq++;
}

int CMultiRequest::IsKeyCompleted(int i) {
	return FD_ISSET(i, (fd_set *)keyMask);
}

int CMultiRequest::DecodeKeyList(void)
{
	if(!wait->FlagMultiKeyVal()) // single task
		return 0;
	
	const CTableDefinition *tableDef = wait->TableDefinition();
	
	keyFields = tableDef->KeyFields();
	if(wait->InternalKeyValList()) {
		// embeded API
		totalReq = wait->InternalKeyValList()->KeyCount();
		// Q&D discard const here
		// this keyList member can't be const,
		// but actually readonly after init
		keyList = (CValue *)&wait->InternalKeyValList()->Value(0,0);
		internal = 1;
	} else {
		// from network
		uint8_t fieldID[keyFields];
		CArray keyNameList(*(wait->KeyNameList()));
		CArray keyValList(*(wait->KeyValList()));
		CBinary keyName;
		for(int i=0; i<keyFields; i++){
			if(keyNameList.Get(keyName) != 0){
				log_error("get key name[%d] error, key field count:%d", i, tableDef->KeyFields());
				return -1;
			}
			fieldID[i] = tableDef->FieldId(keyName.ptr);
		}
		if(keyNameList.Get(keyName) == 0){
			log_error("bogus key name: %.*s", keyName.len, keyName.ptr);
			return -1;
		}

		totalReq = wait->versionInfo.GetTag(11)->u64;
		keyList = (CValue *)MALLOC(totalReq*keyFields*sizeof(CValue));
		for(int i=0; i<totalReq; i++){
			CValue *keyVal = GetKeyValue(i);
			for(int j=0; j<keyFields; j++){
				int fid = fieldID[j];
				switch(tableDef->FieldType(fid)){
					case DField::Signed:
					case DField::Unsigned:
						if(keyValList.Get(keyVal[fid].u64) != 0){
							log_error("get key value[%d][%d] error", i, j);
							return -2;
						}
						break;

					case DField::String:
					case DField::Binary:
						if(keyValList.Get(keyVal[fid].bin) != 0){
							log_error("get key value[%d][%d] error", i, j);
							return -2;
						}
						break;

					default:
						log_error("invalid key type[%d][%d]", i, j);
						return -3;
				}
			}
		}
	}
	
//	keyMask = (unsigned char *)CALLOC(1, (totalReq*keyFields+7)/8);
//	8 bytes aligned waste some memory. FD_SET operate memory by 8bytes
	keyMask = (unsigned char *)CALLOC(8, (((totalReq*keyFields+7)/8)+7)/8);
	return totalReq;
}

int CMultiRequest::SplitTask(void)
{
	for(int i=0; i<totalReq; i++) {
		if(IsKeyCompleted(i))
			continue;

		CValue *keyVal = GetKeyValue(i);
		CTaskRequest *pTask = new CTaskRequest;
		if(pTask == NULL) {
			log_error("%s: %m", "new task error");
			return -1;
		}
		if(pTask->Copy(*wait, keyVal) < 0) {
			log_error("copy task error: %s", pTask->resultInfo.ErrorMessage());
			delete pTask;
			return -1;
		}

		pTask->SetOwnerInfo(this, i, wait->OwnerAddress());
		pTask->PushReplyDispatcher(&multiTaskReply);
		owner->PushTaskQueue(pTask);
		subReq++;
	}
	
	return 0;
}

void CMultiRequest::CompleteTask(CTaskRequest *req, int index)
{
	if(wait) {
		if(wait->ResultCode() >= 0 && req->ResultCode() < 0) { 
			wait->SetErrorDup(req->resultInfo.ResultCode(), req->resultInfo.ErrorFrom(), req->resultInfo.ErrorMessage());
		}

		int ret;
		if((ret = wait->MergeResult(*req)) != 0){
			wait->SetError(ret, "multi_request", "merge result error");
		}
	}
	
	delete req;

	SetKeyCompleted(index);
	subReq--;

	// 注意，如果将CTaskMultiplexer放到cache线程执行，则会导致每split一个task，都是直接到cache_process执行完到这里；然后再split出第二个task。这会导致这一个判断逻辑有问题。
	// 目前CTaskMultiplexer是跟incoming线程绑在一起的，因此没有问题
	if(firstPass==0 && subReq==0)
	{
		CompleteWaiter();
		delete this;
	}
}

void CMultiRequest::CompleteWaiter(void) {
	if(wait) {
		wait->ReplyNotify();
		wait = 0;
	}
}
	
void CMultiRequest::SecondPass(int err)
{ 
	firstPass = 0;
	if(subReq==0) {
		// no sub-request present, complete whole request
		CompleteWaiter();
		delete this;
	} else if(err) {
		// mark all request is done except sub-requests
		doneReq = totalReq - subReq;
		CompleteWaiter();
	}
	return;
}

int CTaskRequest::SetBatchCursor(int index) {
	int err = 0;

	CMultiRequest *mreq = GetBatchKeyList();
	if(mreq==NULL) return -1;

	if(index < 0 || index >= mreq->TotalCount() ) {
		key = NULL;
		multiKey = NULL;
		return -1;
	} else {
		CValue *keyVal = mreq->GetKeyValue(index);
		int kf = TableDefinition()->KeyFields();

		/* switch RequestKey() */
		key = &keyVal[0];
		if(kf > 1) {
			/* switch multi-fields key */
			multiKey = keyVal;
		}
		err = BuildPackedKey();
		if(err < 0)
		{
		    log_error("build packed key error, error from: %s, error message: %s", resultInfo.ErrorFrom(), resultInfo.ErrorMessage());
		    return -1;
		}
	}
	return 0;
}

void CTaskRequest::DoneBatchCursor(int index) {
	CMultiRequest *mreq = GetBatchKeyList();
	if(mreq==NULL) return;

	mreq->SetKeyCompleted(index);
}

