#ifndef __HELPER_GROUP_H__
#define __HELPER_GROUP_H__

#include "lqueue.h"
#include "poll_thread.h"

#include "list.h"
#include "value.h"
#include "request_base.h"
#include "StatTTC.h"

class CHelperClient;
class CHelperClientList;
class CTaskRequest;

class CHelperGroup : private CTimerObject, public CTaskDispatcher<CTaskRequest>
{
public:
	CHelperGroup (const char *sockpath, const char *name, int hc, int qs,
            int statIndex);
	~CHelperGroup ();

	int QueueFull(void) const { return queue.Count() >= queueSize; }
	int QueueEmpty(void) const { return queue.Count() == 0; }
	int HasFreeHelper(void) const { return queue.Count()==0 && freeHelper.ListEmpty(); }

	/* process or queue a task */
	virtual void TaskNotify(CTaskRequest *);

	int Attach(CPollThread *, CLinkQueue<CTaskRequest *>::allocator *);
	int ConnectHelper (int);
	const char *SockPath(void) const{return sockpath;}


	void SetTimerHandler(CTimerList *recv, CTimerList *conn, CTimerList *retry) {
		recvList = recv;
		connList = conn;
		retryList = retry;
		AttachTimer(retryList);
	}

	void QueueBackTask(CTaskRequest *);
	void RequestCompleted (CHelperClient *);
	void ConnectionReset (CHelperClient *);
	void CheckQueueExpire(void);
	void DumpState(void);

	void AddReadyHelper();
	void DecReadyHelper();

private:
	virtual void TimerNotify(void);
	/* trying pop task and process */
	void FlushTask(uint64_t time);
	/* process a task, must has free helper */
	void ProcessTask (CTaskRequest *t);
	const char *Name() const { return name; }
	void RecordResponseDelay(unsigned int t);
	int  AcceptNewRequestFail(CTaskRequest*);
public:
	CTimerList *recvList;
	CTimerList *connList;
	CTimerList *retryList;

private:
	CLinkQueue<CTaskRequest *> queue;
	int queueSize;

	int helperCount;
	int helperMax;
	int readyHelperCnt;
    char *sockpath;
	char name[24];

	CHelperClientList *helperList;
	CListObject<CHelperClientList> freeHelper;
public:
	CHelperGroup *fallback;
public:
	void RecordProcessTime(int type, unsigned int msec);
	/* queue当前长度 */
	int  QueueCount(void) const { return queue.Count(); }
	/* queue最大长度*/
	int  QueueMaxCount(void) const { return queueSize; }
private:
	/* 平均请求时延 */
	double average_delay;

	CStatSample statTime[6];
};

#endif
