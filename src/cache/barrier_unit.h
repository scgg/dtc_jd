#ifndef __BARRIER_UNIT_H__
#define __BARRIER_UNIT_H__

#include <stdint.h>
#include <list.h>
#include "task_request.h"
#include "timerlist.h"
#include "barrier.h"
#include "StatTTC.h"

#define BARRIER_HASH_MAX	1024 * 8

class CTaskRequest;
class CPollThread;
class CBarrierUnit;

class CBarrierUnit : public CTaskDispatcher<CTaskRequest>, public CReplyDispatcher<CTaskRequest> {
public:
	enum E_BARRIER_UNIT_PLACE { IN_FRONT, IN_BACK };
	CBarrierUnit (CPollThread *, int max, int maxkeycount, E_BARRIER_UNIT_PLACE place);
	~CBarrierUnit ();
	virtual void TaskNotify(CTaskRequest *);
	virtual void ReplyNotify(CTaskRequest *);

	void ChainRequest(CTaskRequest *p) {
		p->PushReplyDispatcher(this);
		output.TaskNotify(p);
	}
	void QueueRequest(CTaskRequest *p) {
		p->PushReplyDispatcher(this);
		output.IndirectNotify(p);
	}
	CPollThread *OwnerThread(void) const { return owner; }
	void AttachFreeBarrier(CBarrier *);
	int MaxCountByKey(void) const { return maxKeyCount; }
	void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
	int BarrierCount() const { return count; }
	
protected:
	int count;
	CLinkQueue<CTaskRequest *>::allocator taskQueueAllocator;
	CListObject<CBarrier> freeList;
	CListObject<CBarrier> hashSlot[BARRIER_HASH_MAX]; 
	int maxBarrier;
	
	CBarrier *GetBarrier (unsigned long key);
	CBarrier *GetBarrierByIdx (unsigned long idx);
	int key2idx (unsigned long key) { return key % BARRIER_HASH_MAX; }
	
private:
	int maxKeyCount;

	CRequestOutput<CTaskRequest> output;
	
	//stat
	CStatItemU32 statBarrierCount;
	CStatItemU32 statBarrierMaxTask;
};

#endif
