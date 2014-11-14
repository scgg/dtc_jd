#include <cache_bypass.h>

class CReplyBypass : public CReplyDispatcher<CTaskRequest>
{
public:
	CReplyBypass(void) {}
	virtual ~CReplyBypass(void);
	virtual void ReplyNotify(CTaskRequest *task);
};

CReplyBypass::~CReplyBypass(void) { }

void CReplyBypass::ReplyNotify(CTaskRequest *task)
{
	if(task->result)
		task->PassAllResult(task->result);
	task->ReplyNotify();
}

static CReplyBypass replyBypass;

CCacheBypass::~CCacheBypass(void)
{
}

void CCacheBypass::TaskNotify(CTaskRequest *cur)
{
	if(cur->IsBatchRequest()) {
		cur->ReplyNotify();
		return;
	}

	if(cur->CountOnly() && (cur->requestInfo.LimitStart() || cur->requestInfo.LimitCount()))
	{
		cur->SetError(-EC_BAD_COMMAND,"CacheBypass","There's nothing to limit because no fields required");
		cur->ReplyNotify();
		return;
	}

	cur->MarkAsPassThru();
	cur->PushReplyDispatcher(&replyBypass);
	output.TaskNotify(cur);
}

