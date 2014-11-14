#include "log.h"
#include "cache_process.h"
#include <daemon.h>


void CCacheReplyNotify::ReplyNotify(CTaskRequest *cur)
{
	owner->ReplyNotify(cur);
}

void CCacheProcess::ReplyNotify(CTaskRequest *cur)
{
	TransationBegin(cur);

	if(cur->ResultCode() < 0)
	{
		cache_process_error(*cur);
	} else if (cur->ResultCode() > 0)
	{
		log_notice("ResultCode() > 0: from %s msg %s", cur->resultInfo.ErrorFrom(), cur->resultInfo.ErrorMessage());
	}
	if (cur->ResultCode() >= 0 && cache_process_reply (*cur) != CACHE_PROCESS_OK)
	{
		if(cur->ResultCode() >= 0)
			cur->SetError (-EC_SERVER_ERROR, "cache_process_reply", LastErrorMessage());

	}

	if(!cur->FlagBlackHole()) {
		/* 如果cache操作有失败，则加入黑名单*/
		unsigned blacksize = cur->PopBlackListSize();
		if(blacksize >0)
		{
			log_debug("add to blacklist, key=%d size=%u",cur->IntKey(), blacksize);
			blacklist->add_blacklist(cur->PackedKey(), blacksize);
		}
	}

	cur->ReplyNotify();

	TransationEnd();

	/* 启动匀速淘汰(deplay purge) */
	Cache.DelayPurgeNotify();
}

void CFlushReplyNotify::ReplyNotify(CTaskRequest *cur)
{
	owner->TransationBegin(cur);
	if(cur->ResultCode() < 0)
	{
		owner->cache_process_error(*cur);
	} else if (cur->ResultCode() > 0)
	{
		log_notice("ResultCode() > 0: from %s msg %s", cur->resultInfo.ErrorFrom(), cur->resultInfo.ErrorMessage());
	}
	if (cur->ResultCode() >= 0 && owner->cache_flush_reply (*cur) != CACHE_PROCESS_OK)
	{
		if(cur->ResultCode() >= 0)
			cur->SetError (-EC_SERVER_ERROR, "cache_flush_reply", owner->LastErrorMessage());
	}
	cur->ReplyNotify();
	owner->TransationEnd();
}

void CCacheProcess::TaskNotify(CTaskRequest *cur)
{
	unsigned blacksize = 0;
	TransationBegin(cur);

	if(cur->ResultCode() < 0) {
		cur->MarkAsHit(); /* mark as hit if result done */
		cur->ReplyNotify();
	} else if(cur->IsBatchRequest()) {
		switch(cache_process_batch (*cur)) {
		default:
			cur->SetError (-EC_SERVER_ERROR, "cache_process", LastErrorMessage());
			cur->MarkAsHit(); /* mark as hit if result done */
			cur->ReplyNotify();
			break;

		case CACHE_PROCESS_OK:
			cur->MarkAsHit(); /* mark as hit if result done */
			cur->ReplyNotify();
			break;

		case CACHE_PROCESS_ERROR:
			if(cur->ResultCode() >= 0)
				cur->SetError (-EC_SERVER_ERROR, "cache_process", LastErrorMessage());
			cur->MarkAsHit(); /* mark as hit if result done */
			cur->ReplyNotify();
			break;
		}
	} else if(nodbMode==false) {
		switch(cache_process_request (*cur)) {
		default:
			if(!cur->FlagBlackHole()) {
				/* 如果cache操作有失败，则加入黑名单*/
				blacksize = cur->PopBlackListSize();
				if(blacksize >0)
				{
					log_debug("add to blacklist, key=%d size=%u",cur->IntKey(), blacksize);
					blacklist->add_blacklist(cur->PackedKey(), blacksize);
				}
			}
		case CACHE_PROCESS_ERROR:
			if(cur->ResultCode() >= 0)
				cur->SetError (-EC_SERVER_ERROR, "cache_process", LastErrorMessage());

		case CACHE_PROCESS_OK:
			cur->MarkAsHit(); /* mark as hit if result done */
			cur->ReplyNotify();
			break;
		case CACHE_PROCESS_NEXT:
			log_debug("push task to next-unit");
			cur->PushReplyDispatcher(&cacheReply);
			output.TaskNotify(cur);
			break;
		case CACHE_PROCESS_PENDING:
			break;
		case CACHE_PROCESS_REMOTE: //migrate 命令，给远端ttc
			cur->PushReplyDispatcher(&cacheReply);
			remoteoutput.TaskNotify(cur);
			break;

		}
	} else {
		switch(cache_process_nodb (*cur)) {
		default:
		case CACHE_PROCESS_ERROR:
			if(cur->ResultCode() >= 0)
				cur->SetError (-EC_SERVER_ERROR, "cache_process", LastErrorMessage());

		case CACHE_PROCESS_NEXT:
		case CACHE_PROCESS_OK:
			cur->MarkAsHit(); /* mark as hit if result done */
			cur->ReplyNotify();
			break;
		case CACHE_PROCESS_PENDING:
			break;
		case CACHE_PROCESS_REMOTE: //migrate 命令，给远端ttc
			cur->PushReplyDispatcher(&cacheReply);
			remoteoutput.TaskNotify(cur);
			break;
		}
	}
	TransationEnd();
	/* 启动匀速淘汰(deplay purge) */
	Cache.DelayPurgeNotify();
}
