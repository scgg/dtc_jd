#ifndef __PIPE_MTQUEUE_NOLOCK_H__
#define __PIPE_MTQUEUE_NOLOCK_H__

#include <unistd.h>
#include <pthread.h>
#include "poller.h"
#include "lqueue.h"
#include "LockFreeQueue.h"
#include "log.h"

#define CAS(a_ptr, a_oldVal, a_newVal) __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

/*
 * 提供给业务类继承
 */
class BaseTask {
private:
	int syncflag;
	int cmd;
public:
	BaseTask():syncflag(0),cmd(0)
	{
		//do nothing
	}
	virtual ~BaseTask()
	{
		//do nothing
	}
	void SetSyncFlag(int flag) {
		this->syncflag = flag;
	}
	int GetSyncFlag() {
		return this->syncflag;
	}
	void SetCmd(int flag) {
		this->cmd = flag;
	}
	int GetCmd() {
		return this->cmd;
	}
};

template<typename C>
class CThreadingPipeNoLockQueue: CPollerObject {
private:

	LockFreeQueue<BaseTask *> queue;
	int wakefd;
	int resp_wakefd;
	int resp_netfd;
	volatile uint32_t queueSize;

private:

	// pipe management
	inline void Wake() {
		char c = 0;
		write(wakefd, &c, 1);
	}

	inline void Response() {
		char c = 0;
		write(resp_wakefd, &c, 1);
	}

	inline void Discard() {
		char buf[1];
		int n;
		n = read(netfd, buf, sizeof(buf));
		log_debug("the byte read from pipe is %d", n);
	}

	// reader implementation
	virtual void HangupNotify(void) {
	}
	virtual void InputNotify(void) {
		BaseTask *p;
		int n = 0;
		log_debug("in the InputNotify the queue size is %d ", Count());
		while (++n <= 64 && queueSize > 0) {
			int ret = queue.DeQueue(p);
			if (true == ret) {
				__sync_fetch_and_sub(&queueSize, 1);
				static_cast<C *>(this)->TaskNotify(p);
				if(p->GetSyncFlag()!=0)
				{
					Response();
				}
			}
			// running task in unlocked mode
		}
		log_debug("in the InputNotify the queue size is %d ", Count());
		if (queueSize <= 0) {
			log_debug("Discard been called %d", Count());
			Discard();
		}
	}

public:
	CThreadingPipeNoLockQueue() :
			wakefd(-1), resp_wakefd(-1), resp_netfd(-1) {
	}
	~CThreadingPipeNoLockQueue() {
	}

	inline int AttachPoller(CPollerUnit *thread) {
		int fd_response[2];
		int ret = pipe(fd_response);
		if (ret != 0)
			return ret;
		resp_wakefd = fd_response[1];
		resp_netfd = fd_response[0];

		int fd_send[2];
		ret = pipe(fd_send);
		if (ret != 0)
			return ret;

		wakefd = fd_send[1];
		netfd = fd_send[0];
		EnableInput();
		return CPollerObject::AttachPoller(thread);
	}

	inline int Push(BaseTask *p) {
		uint32_t qsz;
		int ret;
		ret = queue.EnQueue(p);
		if (true == ret) {
			qsz = __sync_fetch_and_add(&queueSize, 1);
			if (qsz == 0)
				Wake();
		}
		if (p->GetSyncFlag() != 0) {
			char buf[1];
			int n;
			n = read(resp_netfd, buf, sizeof(buf));
			log_debug("resp from other thread!");
		}
		return ret;
	}

	inline int Count(void) {
		return queueSize;
	}

	inline int QueueEmpty(void) {
		return Count() == 0;
	}
};

#endif
