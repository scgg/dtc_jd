#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>

#include "list.h"
#include "dbconfig.h"
#include "helper_client.h"
#include "helper_group.h"
#include "log.h"
#include "unix_socket.h"

class CHelperClientList: public CListObject<CHelperClientList> {
	public:
		CHelperClientList() : helper(NULL) {}
		~CHelperClientList() { DELETE(helper); }
		CHelperClient *helper;
};

CHelperGroup::CHelperGroup(const char *s, const char *name_, int hc, int qs,
        int statIndex):
	CTaskDispatcher<CTaskRequest>(NULL),
	queueSize(qs),
	helperCount (0),
	helperMax (hc),
	readyHelperCnt(0),
	fallback(NULL),
	average_delay(0) /*默认时延为0*/
{
    sockpath = strdup(s);
	freeHelper.InitList();
	helperList = new CHelperClientList[hc];

	recvList = connList = retryList = NULL;

    strncpy(name, name_, sizeof(name) - 1);
    name[sizeof(name)-1] = '0';

	statTime[0] = statmgr.GetSample(statIndex);
	statTime[1] = statmgr.GetSample(statIndex + 1);
	statTime[2] = statmgr.GetSample(statIndex + 2);
	statTime[3] = statmgr.GetSample(statIndex + 3);
	statTime[4] = statmgr.GetSample(statIndex + 4);
	statTime[5] = statmgr.GetSample(statIndex + 5);
}

CHelperGroup::~CHelperGroup ()
{
	DELETE_ARRAY(helperList);
    free(sockpath);
}


void CHelperGroup::RecordResponseDelay(unsigned int t)
{
	if(t <= 0)
		t=1;

	if(average_delay == 0)
		average_delay = t;

	double N = 20E6/(average_delay+t);

	if((unsigned)N > 200000)
		N = 200000; /* 2w/s */
	if((unsigned)N < 5)
		N = 5;	    /* 0.5/s */

	average_delay = ((N-1)/N)*average_delay + t/N;
}

void CHelperGroup::AddReadyHelper()
{
	if(readyHelperCnt == 0)
	{
		log_info("helper_group-%s switch to ONLINE mode", sockpath);
		/* force flush task */
		AttachReadyTimer(owner);
	}

	if(readyHelperCnt < helperMax)
		readyHelperCnt++;

	if(readyHelperCnt == helperMax)
		log_debug("%s", "all client ready");
}

void CHelperGroup::DecReadyHelper()
{
	if(readyHelperCnt > 0)
		readyHelperCnt--;
	if(readyHelperCnt == 0)
	{
		log_error("helper_group-%s all clients invalid, switch to OFFLINE mode", sockpath);
		/* reply all task */
		AttachReadyTimer(owner);
	}
}

void CHelperGroup::RecordProcessTime(int cmd, unsigned int usec)
{
	static const unsigned char cmd2type[] =
	{
		/*Nop*/ 0,
		/*ResultCode*/ 0,
		/*ResultSet*/ 0,
		/*HelperAdmin*/ 0,
		/*Get*/ 1,
		/*Purge*/ 0,
		/*Insert*/ 2,
		/*Update*/ 3,
		/*Delete*/ 4,
		/*Replace*/ 5,
		/*Flush*/ 0,
		/*Other*/ 0,
		/*Other*/ 0,
		/*Other*/ 0,
		/*Other*/ 0,
		/*Other*/ 0,
	};
	statTime[0].push(usec);
	int t = cmd2type[cmd];
	if(t) statTime[t].push(usec);

	/* 计算新的平均时延 */
	RecordResponseDelay(usec);
}

int CHelperGroup::Attach(CPollThread *thread, CLinkQueue<CTaskRequest *>::allocator *a)
{
	owner = thread;
	for (int i = 0; i < helperMax; i++)
	{
		helperList[i].helper = new CHelperClient(owner, this, i);
		helperList[i].helper->Reconnect();
	}

	queue.SetAllocator(a);
	return 0;
}

int CHelperGroup::ConnectHelper (int fd)
{
	struct sockaddr_un unaddr;
	socklen_t addrlen;

	addrlen = InitUnixSocketAddress(&unaddr, sockpath);
	return connect (fd, (struct sockaddr *)&unaddr, addrlen);
}

void CHelperGroup::QueueBackTask(CTaskRequest *task) {
	if (queue.Unshift(task) < 0) {
		task->SetError(-EC_SERVER_ERROR, __FUNCTION__, "insufficient memory");
		task->ReplyNotify();
	}
}

void CHelperGroup::RequestCompleted (CHelperClient *h)
{
	CHelperClientList *h0 = &helperList[h->helperIdx];
	if(h0->ListEmpty()) {
		h0->ListAdd(freeHelper);
		helperCount --;
		AttachReadyTimer(owner);
	}
}

void CHelperGroup::ConnectionReset (CHelperClient *h)
{
	CHelperClientList *h0 = &helperList[h->helperIdx];
	if(!h0->ListEmpty()) {
		h0->ListDel();
		helperCount ++;
	}
}

void CHelperGroup::CheckQueueExpire(void)
{
	AttachReadyTimer(owner);
}

void CHelperGroup::ProcessTask (CTaskRequest *task)
{
	CHelperClientList *h0 = freeHelper.NextOwner();
	CHelperClient *helper = h0->helper;

	log_debug("process task.....");
	if(helper->SupportBatchKey())
		task->MarkFieldSetWithKey();

	CPacket *packet = new CPacket;
	if (packet->EncodeForwardRequest(task) != 0)
	{
		delete packet;
		log_error("[2][task=%d]request error: %m", task->Role());
		task->SetError (-EC_BAD_SOCKET, "ForwardRequest", NULL);
		task->ReplyNotify();
	} else {
		h0->ResetList();
		helperCount ++;

		helper->AttachTask(task, packet);
	}
}

void CHelperGroup::FlushTask (uint64_t now) {
	//check timeout for helper client
	while(1) {
		CTaskRequest *task = queue.Front();
		if(task==NULL)
			break;

		if(readyHelperCnt == 0) {
			log_debug("no available helper, up stream server maybe offline");
			queue.Pop();
			task->SetError(-EC_UPSTREAM_ERROR, __FUNCTION__,
					"no available helper, up stream server maybe offline");
			task->ReplyNotify();
			continue;
		}else if(task->IsExpired(now)) {
			log_debug("%s", "task is expired in HelperGroup queue");
			queue.Pop();
			task->SetError(-ETIMEDOUT, __FUNCTION__, "task is expired in HelperGroup queue");
			task->ReplyNotify();
		}else if(!freeHelper.ListEmpty()) {
			queue.Pop();
			ProcessTask(task);
		}else if(fallback && fallback->HasFreeHelper()) {
			queue.Pop();
			fallback->ProcessTask(task);
		}
		else {
			break;
		}
	}
}

void CHelperGroup::TimerNotify(void) {
	uint64_t v = GET_TIMESTAMP()/1000;
	AttachTimer(retryList);
	FlushTask(v);
}

int CHelperGroup::AcceptNewRequestFail(CTaskRequest *task)
{
	unsigned work_client = helperMax;
	unsigned queue_size  = queue.Count();

	/* queue至少排队work_client个任务 */
	if(queue_size <= work_client)
		return 0;

	uint64_t wait_time = queue_size * (uint64_t)average_delay;
	wait_time /= work_client; /* us */
	wait_time /= 1000;     /* ms */

	uint64_t now = GET_TIMESTAMP()/1000;

	if(task->IsExpired(now+wait_time))
		return 1;

	return 0;
}

void CHelperGroup::TaskNotify (CTaskRequest *task)
{
	log_debug("CHelperGroup::TaskNotify()");

	uint64_t now = GET_TIMESTAMP()/1000;  /* ms*/
	FlushTask(now);

	if(readyHelperCnt == 0){
		log_debug("no available helper, up stream server maybe offline");
		task->SetError(-EC_UPSTREAM_ERROR, __FUNCTION__, "no available helper, up stream server maybe offline");
		task->ReplyNotify();
    } else if(task->IsExpired(now)) {
        log_error("task is expired when sched to HelperGroup");
        //modify error message
        //tapd:644841 by foryzhou
        task->SetError(-ETIMEDOUT, __FUNCTION__, "task is expired when sched to HelperGroup,by TTC");
		task->ReplyNotify();
	} else if(!freeHelper.ListEmpty()) {
		/* has free helper, sched task */
		ProcessTask(task);
	} else {
		if(fallback && fallback->HasFreeHelper()){
			fallback->ProcessTask(task);
		}
		else if(AcceptNewRequestFail(task))
		{
			/* helper 响应变慢，主动踢掉task */
			log_error("HelperGroup response is slow, give up current task");
			task->SetError(-EC_SERVER_BUSY, __FUNCTION__, "DB response is very slow, give up current task");
			task->ReplyNotify();
		}
		else if (!QueueFull()){
			queue.Push(task);
		}
		else {
			/* no free helper */
			task->SetError (-EC_SERVER_BUSY, __FUNCTION__, "No available helper connections");
			log_error("No available helper queue slot,count=%d, max=%d",
					queue.Count(), queueSize);

			task->ReplyNotify();
		}
	}
}

void CHelperGroup::DumpState(void) {
	log_info("HelperGroup %s count %d/%d", Name(), helperCount, helperMax);
	int i;
	for(i=0; i < helperMax; i++) {
		log_info("helper %d state %s\n", i, helperList[i].helper->StateString());
	}
}

