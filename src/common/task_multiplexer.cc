#include "task_request.h"
#include "task_multiplexer.h"
#include "log.h"

class CReplyMultiplexer : public CReplyDispatcher<CTaskRequest>
{
public:
	CReplyMultiplexer(void) {}
	virtual ~CReplyMultiplexer(void);
	virtual void ReplyNotify(CTaskRequest *task);
};

CReplyMultiplexer::~CReplyMultiplexer(void) { }

void CReplyMultiplexer::ReplyNotify(CTaskRequest *cur)
{
	CMultiRequest* req = cur->GetBatchKeyList();
	/* reset BatchKey state */
	cur->SetBatchCursor(-1);
	if(cur->ResultCode() < 0) {
		req->SecondPass(-1);
	} else if(req->RemainCount() <= 0) {
		req->SecondPass(0);
	} else if(req->SplitTask() != 0){
		log_error("split task error");
		cur->SetError(-ENOMEM, __FUNCTION__, "split task error");
		req->SecondPass(-1);
	} else {
		req->SecondPass(0);
	}
}

static CReplyMultiplexer replyMultiplexer;

CTaskMultiplexer::~CTaskMultiplexer(void)
{
}

void CTaskMultiplexer::TaskNotify(CTaskRequest *cur)
{
	
	if(!cur->FlagMultiKeyVal()){
		// single task, no dispatcher needed
		output.TaskNotify(cur);
		return;
	}
	
#if 0
	// multi-task
	if(cur->FlagPassThru()){
		log_error("multi-task not support under pass-through mode");
		cur->SetError(-EC_TOO_MANY_KEY_VALUE, __FUNCTION__, "multi-task not support under pass-through mode");
		cur->ReplyNotify();
		return;
	}
#endif

	switch(cur->RequestCode()){
		case DRequest::Insert:
		case DRequest::Replace:
			if(cur->TableDefinition()->HasAutoIncrement()){
				log_error("table has autoincrement field, multi-insert/replace not support");
				cur->SetError(-EC_TOO_MANY_KEY_VALUE, __FUNCTION__, "table has autoincrement field, multi-insert/replace not support");
				cur->ReplyNotify();
				return;
			}
			break;

		case DRequest::Get:
			if(cur->requestInfo.LimitStart()!=0 || cur->requestInfo.LimitCount()!=0) {
				log_error("multi-task not support Limit()");
				cur->SetError(-EC_TOO_MANY_KEY_VALUE, __FUNCTION__, "multi-task not support Limit()");
				cur->ReplyNotify();
				return;
			}
		case DRequest::Delete:
		case DRequest::Purge:
		case DRequest::Update:
		case DRequest::Flush:
			break;

		default:
			log_error("multi-task not support other than get/insert/update/delete/purge/replace/flush request");
			cur->SetError(-EC_TOO_MANY_KEY_VALUE, __FUNCTION__, "bad batch request type");
			cur->ReplyNotify();
			return;
	}

	CMultiRequest* req = new CMultiRequest(this, cur);
	if(req == NULL){
		log_error("new CMultiRequest error: %m");
		cur->SetError(-ENOMEM, __FUNCTION__, "new CMultiRequest error");
		cur->ReplyNotify();
		return;
	}

	if(req->DecodeKeyList() <= 0) {
		/* empty batch or errors */
		cur->ReplyNotify();
		delete req;
		return;
	}

	cur->SetBatchKeyList(req);
	if(fastUpdate || cur->RequestCode() == DRequest::Get) {
		// fast batch path, batch hits, split task fallback
		cur->PushReplyDispatcher(&replyMultiplexer);
		output.TaskNotify(cur);
	} else
		// slow batch path, split task
		replyMultiplexer.ReplyNotify(cur);
	return;
}

