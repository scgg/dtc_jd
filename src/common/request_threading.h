#ifndef __H_TTC_REQUEST_THREAD_H__
#define __H_TTC_REQUEST_THREAD_H__

#include "mtpqueue.h"
#include "waitqueue.h"
#include "request_base_all.h"


template<typename T>
class CThreadingOutputDispatcher
{
private: // internal implementation
	class CInternalTaskDispatcher: public CThreadingPipeQueue<T *, CInternalTaskDispatcher> {
	public:
		CTaskDispatcher<T> *proc;
	public:
		CInternalTaskDispatcher() : proc(0) {}
		virtual ~CInternalTaskDispatcher() {}
		void TaskNotify(T *p) {
			proc->TaskNotify(p);
		};
	};

	class CInternalReplyDispatcher: public CReplyDispatcher<T>, public CThreadingWaitQueue<T *>
	{
	public:
		CInternalReplyDispatcher *freenext;
		CInternalReplyDispatcher *allnext;
	public:
		CInternalReplyDispatcher() : freenext(0), allnext(0) {}
		virtual ~CInternalReplyDispatcher() {}
		virtual void ReplyNotify(T *p) {
			Push(p);
		};
	};

private:
	CInternalTaskDispatcher incQueue;
	pthread_mutex_t lock;
	// lock management, protect freeList and allList
	inline void Lock(void) { pthread_mutex_lock(&lock); }
	inline void Unlock(void) { pthread_mutex_unlock(&lock); }
	CInternalReplyDispatcher * volatile freeList;
	CInternalReplyDispatcher * volatile allList;
	volatile int stopping;

public:
	CThreadingOutputDispatcher() : freeList(0), allList(0), stopping(0) {
		pthread_mutex_init(&lock, NULL);
	}
	~CThreadingOutputDispatcher() {
		CInternalReplyDispatcher *q;
		while(allList) {
			q = allList;
			allList = q->allnext;
			delete q;
		}
		pthread_mutex_destroy(&lock);
	}

	void Stop(void) {
		stopping = 1;
		CInternalReplyDispatcher *p;
		for(p=allList; p; p=p->allnext)
			p->Stop(NULL);
	}

	int Stopping(void) { return stopping; }

	int Execute(T *p) {
		CInternalReplyDispatcher *q;

		// aborted without side-effect
		if(Stopping())
		    return -1;

		/* freelist被别的线程在lock锁住的时候被别的线程置成了NULL */
		Lock();
		if(freeList){
		    q = freeList;
		    freeList = q->freenext;
		    q->Clear();
		    q->freenext = NULL;
		}
		else
		{
		    q = new CInternalReplyDispatcher();
		    q->allnext = allList;
		    allList = q;
		}
		Unlock();

		p->PushReplyDispatcher(q);
		incQueue.Push(p);
		// has side effect
		if(q->Pop() == NULL) {
			// q leaked by purpose
			// because an pending reply didn't Popped
			return -2;
		}

		Lock();
		q->freenext = freeList;
		freeList = q;
		Unlock();
		return 0;
	}

	int BindDispatcher(CTaskDispatcher<T> *to)
	{
		log_debug("Create ThreadingOutputDispatcher to thread %s",
				to->OwnerThread()->Name());
		incQueue.proc = to;
		return incQueue.AttachPoller(to->OwnerThread());
	}
};

#endif
