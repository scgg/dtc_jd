#include <stdio.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "client_sync.h"
#include "task_request.h"
#include "packet.h"
#include "client_resource_pool.h"
#include "client_unit.h"
#include "log.h"

#include <StatTTC.h>

class CPollThread;
static int statEnable=0;
static CStatItemU32 statAcceptCount;
static CStatItemU32 statCurConnCount;

class CReplySync : public CReplyDispatcher<CTaskRequest>
{
	public:
		CReplySync(void) {}
		virtual ~CReplySync(void) {}
		virtual void ReplyNotify(CTaskRequest *task);
};

void CReplySync::ReplyNotify(CTaskRequest *task)
{
	CClientSync *cli = task->OwnerInfo<CClientSync>();
	CTTCDecoderUnit * resourceOwner = (CTTCDecoderUnit *)task->resourceOwner;

	/* cli not exist, clean&free resource */
	if (cli == NULL)
	{
	    if(task->resourceId != 0)
	    {
		resourceOwner->CleanResource(task->resourceId);
		resourceOwner->UnregistResource(task->resourceId, task->resourceSeq);
	    }
	}
	/* normal case */
	else if(cli->SendResult()==0)
	{
	    cli->DelayApplyEvents();
	} else /* send error, delete client, resource will be clean&free there */
	{
		// delete client apply delete task
		delete cli;
	}
}

static CReplySync syncReplyObject;

CClientSync::CClientSync(CTTCDecoderUnit *o, int fd, void *peer, int ps):
	CPollerObject (o->OwnerThread(), fd),
	owner(o),
	receiver(fd),
	stage(IdleState),
	task(NULL),
	reply(NULL)
{

	addrLen = ps;
	addr    = MALLOC(ps);
	resourceId = 0;
	resource = NULL;
	
	if(addr == NULL)
        throw (int) -ENOMEM;

    memcpy((char *)addr, (char *)peer, ps);

    if(!statEnable){
        statAcceptCount = statmgr.GetItemU32(ACCEPT_COUNT);
        statCurConnCount = statmgr.GetItemU32(CONN_COUNT);
        statEnable = 1;
    }

    /* CClientSync deleted if allocate resource failed. clean resource allocated */
    GetResource();
    if(resource)
    {
        task = resource->task;
        reply = resource ->packet;
    }else
    {
        throw (int) -ENOMEM;
    }
    rscStatus = RscClean;

    ++statAcceptCount;
    ++statCurConnCount;
}

CClientSync::~CClientSync ()
{    
	if(task) {
		if(stage == ProcReqState)
		{
			/* task in use, save resource to reply */
			task->ClearOwnerInfo();
			task->resourceId = resourceId;
			task->resourceOwner = owner;
			task->resourceSeq = resourceSeq;
		}else
		{
		    /* task not in use, clean resource, free resource */
		    if(resource)
		    {
			CleanResource();
			rscStatus = RscClean;
			FreeResource();
		    }
		}
	}

	//DELETE(reply);
	FREE_IF(addr);
	--statCurConnCount;
}

int CClientSync::Attach ()
{
	EnableInput();
	if (AttachPoller () == -1)
		return -1;

	AttachTimer (owner->IdleList());

	stage = IdleState;
	return 0;
}

//server peer
int CClientSync::RecvRequest ()
{
    /*
       task = new CTaskRequest(owner->OwnerTable());
       if (NULL == task)
    {
	log_error("create CTaskRequest object failed, msg[no enough memory]");
	return -1;
    }
    */

    /* clean task from pool, basic init take place of old construction */
    if(RscClean == rscStatus)
    {
	task->SetDataTable(owner->OwnerTable());
	task->SetHotbackupTable(owner->AdminTable());
	task->SetRoleAsServer();
	task->BeginStage();
	receiver.erase();
	rscStatus = RscDirty;
    }

    DisableTimer();

    int ret = task->Decode (receiver);
    switch (ret) 
    {
    default:
    case DecodeFatalError:
        if(errno != 0)
        log_notice("decode fatal error, ret = %d msg = %m", ret);
        return -1;

    case DecodeDataError:
        task->ResponseTimerStart();
        task->MarkAsHit();
        if(task->ResultCode() < 0)
            log_notice("DecodeDataError, role=%d, fd=%d, result=%d", task->Role (), netfd, task->ResultCode ());
        return SendResult();

    case DecodeIdle:
        AttachTimer(owner->IdleList());
        stage = IdleState;        
        rscStatus = RscClean;
        break;

    case DecodeWaitData:
        stage = RecvReqState;
        break;

    case DecodeDone:
        if((ret = task->PrepareProcess()) < 0){
        log_debug("build packed key error: %d", ret);
        return SendResult();
        }
#if 0
        /* 处理任务时，如果client关闭连接，server也应该关闭连接, 所以依然EnableInput */
        DisableInput();
#endif
        DisableOutput();
        task->SetOwnerInfo(this, 0, (struct sockaddr *)addr);
        stage = ProcReqState;
        task->PushReplyDispatcher(&syncReplyObject);
        owner->TaskDispatcher(task);
    }
    return 0;
}

/* keep task in resource slot */
int CClientSync::SendResult (void) {
	stage = SendRepState;

	owner->RecordRequestTime(task);

	//reply = new CPacket;
/*
	if(reply == NULL)
	{
		log_error("create CPacket object failed");
		return -1;
	}
*/
	if(task->FlagKeepAlive())
		task->versionInfo.SetKeepAliveTimeout(owner->IdleTime());
	reply->EncodeResult (task);

	//DELETE(task);

	return Response();
}

int CClientSync::Response (void)
{
	int ret = reply->Send (netfd);   

	switch (ret)
	{
		case SendResultMoreData:
			EnableOutput();
			return 0;

		case SendResultDone:
			CleanResource();
			rscStatus = RscClean;
			stage = IdleState;
			DisableOutput();
			EnableInput();
			AttachTimer(owner->IdleList());
			return 0;

		default:
			log_notice("send failed, return = %d, error = %m", ret);
			return -1;
	}
	return 0;
}

void CClientSync::InputNotify (void)
{
	if (stage==IdleState || stage==RecvReqState) {
		if(RecvRequest () < 0)
		{

			delete this;
			return;
		}

	}

	/* receive input events again. */
	else
	{
		/*  check whether client close connection. */
		if(CheckLinkStatus())
		{
			log_debug("client close connection, delete CClientSync obj, stage=%d", stage);
			delete this;
			return;
		}
		else
		{
			DisableInput();
		}
	}

	DelayApplyEvents();
}

void CClientSync::OutputNotify (void)
{
	if (stage==SendRepState) {
		if(Response () < 0)
			delete this;
	} else {
		DisableOutput();
		log_info("Spurious CClientSync::OutputNotify, stage=%d", stage);
	}
}

int CClientSync::GetResource()
{
    return owner->RegistResource(&resource, resourceId, resourceSeq);
}

void CClientSync::FreeResource()
{
    task = NULL;
    reply = NULL;
    owner->UnregistResource(resourceId, resourceSeq);
}

void CClientSync::CleanResource()
{
    owner->CleanResource(resourceId);
}
