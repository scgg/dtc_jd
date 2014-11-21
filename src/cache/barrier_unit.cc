#include <stdio.h>

#include <barrier.h>
#include <barrier_unit.h>
#include <poll_thread.h>

#include "log.h"

//-------------------------------------------------------------------------
CBarrierUnit::CBarrierUnit (CPollThread *o, int max, int maxkeycount, E_BARRIER_UNIT_PLACE place) :
	CTaskDispatcher<CTaskRequest>(o),
	count(0),
	maxBarrier (max),
	maxKeyCount (maxkeycount),
	output(o)
{
	freeList.InitList();
	for (int i = 0; i < BARRIER_HASH_MAX; i++)
		hashSlot[i].InitList();
	//stat
	if (IN_FRONT == place)
	{
		statBarrierCount = statmgr.GetItemU32(DTC_FRONT_BARRIER_COUNT);
		statBarrierMaxTask = statmgr.GetItemU32(DTC_FRONT_BARRIER_MAX_TASK);
	}
	else if (IN_BACK == place)
	{
		statBarrierCount = statmgr.GetItemU32(DTC_BACK_BARRIER_COUNT);
		statBarrierMaxTask = statmgr.GetItemU32(DTC_BACK_BARRIER_MAX_TASK);
	}
	else
	{
		log_error("bad place value %d", place);
	}
	statBarrierCount = 0;
	statBarrierMaxTask = 0;
}

CBarrierUnit::~CBarrierUnit ()
{
	while(!freeList.ListEmpty()) {
		delete static_cast<CBarrier *>(freeList.ListNext());
	}
	for (int i = 0; i < BARRIER_HASH_MAX; i++) {
		while(!hashSlot[i].ListEmpty()) {
			delete static_cast<CBarrier *>(hashSlot[i].ListNext());
		}
	}
}

CBarrier *CBarrierUnit::GetBarrier (unsigned long key)
{
	CListObject<CBarrier> *h = &hashSlot[key2idx(key)];
	CListObject<CBarrier> *p;

	for(p = h->ListNext(); p != h; p = p->ListNext())
	{
		if (p->ListOwner()->Key () == key)
			return p->ListOwner();
	}

	return NULL;
}

CBarrier *CBarrierUnit::GetBarrierByIdx (unsigned long idx)
{
	if(idx >= BARRIER_HASH_MAX)
		return NULL;
		
	CListObject<CBarrier> *h = &hashSlot[idx];
	CListObject<CBarrier> *p;

	p = h->ListNext();
	return p->ListOwner();
}

void CBarrierUnit::AttachFreeBarrier(CBarrier *bar) {
	bar->ListMove(freeList);
	count--;
	statBarrierCount = count;
        //Stat.set_barrier_count (count);
}

void CBarrierUnit::TaskNotify(CTaskRequest *cur) {
	if(cur->RequestCode() == DRequest::SvrAdmin && 
		cur->requestInfo.AdminCode() != DRequest::ServerAdminCmd::Migrate){
			//Migrate命令在PrepareRequest的时候已经计算了PackedKey和hash，需要跟普通的task一起排队
		ChainRequest(cur);
		return;
	}
	if(cur->IsBatchRequest()){
		ChainRequest(cur);
		return;
	}

	unsigned long key = cur->BarrierKey();
	CBarrier* bar = GetBarrier(key);

	
	if(bar) {
		if(bar->Count() < maxKeyCount)
		{
			bar->Push(cur);
			if (bar->Count() > statBarrierMaxTask) //max key number
				statBarrierMaxTask = bar->Count();
		}
		else {
			log_warning("barrier[%s]: overload max key count %d bars %d", owner->Name(), maxKeyCount, count);
			cur->SetError(-EC_SERVER_BUSY, __FUNCTION__,
				"too many request blocked at key");
			cur->ReplyNotify();
		}
	} else if(count >= maxBarrier) {
		log_warning("too many barriers, count=%d", count);
		cur->SetError(-EC_SERVER_BUSY, __FUNCTION__,
				"too many barriers");
		cur->ReplyNotify();
	} else {
		if(freeList.ListEmpty()) {
			bar = new CBarrier(&taskQueueAllocator);
		} else {
			bar = freeList.NextOwner();
		}
		bar->SetKey(key);
		bar->ListMoveTail(hashSlot[key2idx(key)]);
		bar->Push(cur);
		count++;
		statBarrierCount = count; //barrier number
		//Stat.set_barrier_count (count);
		ChainRequest(cur);
	}
}

void CBarrierUnit::ReplyNotify(CTaskRequest *cur) {
	if(cur->RequestCode() == DRequest::SvrAdmin &&
		cur->requestInfo.AdminCode() != DRequest::ServerAdminCmd::Migrate){
		cur->ReplyNotify();
		return;
	}
	if(cur->IsBatchRequest()){
		cur->ReplyNotify();
		return;
	}
	
	unsigned long key = cur->BarrierKey();
	CBarrier* bar = GetBarrier(key);
	if(bar==NULL) {
		log_error("return task not in barrier, key=%lu", key);
	} else if(bar->Front()==cur) {
		if (bar->Count() == statBarrierMaxTask) //max key number
				statBarrierMaxTask--;
		bar->Pop();
		CTaskRequest *next = bar->Front();
		if(next == NULL) {
			AttachFreeBarrier(bar);
		} else {
			QueueRequest(next);
		}
//printf("pop bar %lu: count %d\n", key, bar->Count());
	} else {
		log_error("return task not barrier header, key=%lu", key);
	}

	cur->ReplyNotify();
}


