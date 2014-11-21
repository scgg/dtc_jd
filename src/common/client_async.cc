#include <stdio.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "client_async.h"
#include "client_unit.h"
#include "poll_thread.h"
#include "log.h"

static int statEnable=0;
static CStatItemU32 statAcceptCount;
static CStatItemU32 statCurConnCount;

inline CAsyncInfo::CAsyncInfo(CClientAsync *c, CTaskRequest *r)
	:
	cli(c),
	req(r),
	pkt(NULL)

{
	c->numReq++;
	/* move to waiting list */
	ListAddTail(&c->waitList);
}

inline CAsyncInfo::~CAsyncInfo()
{
	if(req)
		req->ClearOwnerInfo();
	DELETE(pkt);
	if(cli)
		cli->numReq--;
}


class CReplyAsync : public CReplyDispatcher<CTaskRequest>
{
public:
	CReplyAsync(void) {}
	virtual ~CReplyAsync(void);
	virtual void ReplyNotify(CTaskRequest *task);
};

CReplyAsync::~CReplyAsync(void) { }

void CReplyAsync::ReplyNotify(CTaskRequest *task)
{
	CAsyncInfo *info = task->OwnerInfo<CAsyncInfo>();
	if (info == NULL) {
		delete task;
	} else if(info->cli == NULL) {
		log_error("info->cli is NULL, possible memory corrupted");
		delete task;
	} else {
		info->cli->QueueResult(info);
	}
}

static CReplyAsync replyAsync;

CClientAsync::CClientAsync(CTTCDecoderUnit *o, int fd, int m, void *peer, int ps)
	:CPollerObject (o->OwnerThread(), fd),
	owner(o),
	receiver(fd),
	curReq(NULL),
	curRes(NULL),
	maxReq(m),
	numReq(0)
{
	addrLen = ps;
	addr = MALLOC(ps);
	memcpy((char *)addr, (char *)peer, ps);

	if(!statEnable){
		statAcceptCount = statmgr.GetItemU32(ACCEPT_COUNT);
		statCurConnCount = statmgr.GetItemU32(CONN_COUNT);
		statEnable = 1;
	}
	++statAcceptCount;
	++statCurConnCount;
}

CClientAsync::~CClientAsync ()
{    
	while(!doneList.ListEmpty())
	{
		CAsyncInfo *info = doneList.NextOwner();
		delete info;
	}
	while(!waitList.ListEmpty())
	{
		CAsyncInfo *info = waitList.NextOwner();
		delete info;
	}
	--statCurConnCount;

    FREE_IF(addr);
}

int CClientAsync::Attach ()
{
	EnableInput();
	if (AttachPoller () == -1)
		return -1;

	return 0;
}

//server peer
int CClientAsync::RecvRequest ()
{
	if(curReq==NULL)
	{
		curReq = new CTaskRequest(owner->OwnerTable());
		if (NULL == curReq)
		{
			log_error("%s", "create CTaskRequest object failed, msg[no enough memory]");
			return -1;
		}
		curReq->SetHotbackupTable(owner->AdminTable());
		receiver.erase();
	}

	int ret = curReq->Decode(receiver);
	switch (ret) 
	{
		default:
		case DecodeFatalError:
			if(errno != 0)
				log_notice("decode fatal error, ret = %d msg = %m", ret);
			DELETE(curReq);
			return -1;

		case DecodeDataError:
			curReq->ResponseTimerStart();
			curReq->MarkAsHit();
			if(curReq->ResultCode() < 0)
				log_notice("DecodeDataError, role=%d, fd=%d, result=%d", 
					curReq->Role (), netfd, curReq->ResultCode ());
			return QueueError();

		case DecodeDone:
			if(curReq->PrepareProcess() < 0)
				return QueueError();
			curReq->SetOwnerInfo( new CAsyncInfo(this, curReq), 0, (struct sockaddr *)addr);
			curReq->PushReplyDispatcher(&replyAsync);
			owner->TaskDispatcher(curReq);
			curReq = NULL;
	}
	return 0;
}

int CClientAsync::QueueError (void)
{
	CAsyncInfo *info = new CAsyncInfo(this, curReq);
	curReq = NULL;
	return QueueResult(info);
}

int CClientAsync::QueueResult (CAsyncInfo *info)
{
	if(info->req==NULL)
	{
		delete info;
		return 0;
	}

	CTaskRequest * const cur = info->req;
	owner->RecordRequestTime(curReq);

	/* move to sending list */
	info->ListMoveTail(&doneList);
	/* convert request to result */
	info->pkt = new CPacket;
	if(info->pkt == NULL)
	{
		delete info->req;
		info->req = NULL;
		delete info;
		log_error("create CPacket object failed");
		return 0;
	}

	cur->versionInfo.SetKeepAliveTimeout(owner->IdleTime());
	info->pkt->EncodeResult (info->req);
	DELETE(info->req);

	Response();
	AdjustEvents();
	return 0;
}

int CClientAsync::ResponseOne(void)
{
	if(curRes==NULL)
		return 0;

	int ret = curRes->Send (netfd);   

	switch (ret)
	{
		case SendResultMoreData:
			return 0;

		case SendResultDone:
			DELETE(curRes);
			numReq--;
			return 0;

		default:
			log_notice("send failed, return = %d, error = %m", ret);
			return -1;
	}
	return 0;
}

int CClientAsync::Response(void)
{
	do {
		int ret;
		if(curRes==NULL)
		{
			// All result sent
			if(doneList.ListEmpty())
				break;

			CAsyncInfo *info = doneList.NextOwner();
			curRes = info->pkt;
			info->pkt = NULL;
			delete info;
			numReq++;
			if(curRes==NULL)
				continue;
		}
		ret = ResponseOne();
		if(ret < 0)
			return ret;
	} while(curRes==NULL);
	return 0;
}

void CClientAsync::InputNotify(void)
{
	if(RecvRequest () < 0)
		delete this;
	else
		AdjustEvents();
}

void CClientAsync::OutputNotify(void)
{
	if(Response() < 0)
		delete this;
	else
		AdjustEvents();
}

int CClientAsync::AdjustEvents (void)
{
	if(curRes==NULL && doneList.ListEmpty())
		DisableOutput();
	else
		EnableOutput();
	if(numReq >= maxReq)
		DisableInput();
	else
		EnableInput();
	return ApplyEvents();
}
