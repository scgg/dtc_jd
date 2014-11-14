
#include <stdint.h>
#include "timerlist.h"

#if TIMESTAMP_PRECISION < 1000
#error TIMESTAMP_PRECISION must >= 1000
#endif

CTimerObject::~CTimerObject(void) {
}

void CTimerObject::TimerNotify(void) {
	delete this;
}

void CTimerObject::AttachTimer(class CTimerList *lst) {
	if(lst->timeout > 0)
		objexp = GET_TIMESTAMP() + lst->timeout * (TIMESTAMP_PRECISION/1000);
	ListMoveTail(lst->tlist);
}

int CTimerList::CheckExpired(int64_t now) {
	int n = 0;
	if(now==0) {
		now = GET_TIMESTAMP();
	}
	while(!tlist.ListEmpty()) {
		CTimerObject *tobj = tlist.NextOwner();
		if(tobj->objexp > now) break;
		tobj->ListDel();
		tobj->TimerNotify();
		n++;
	}
	return n;
}

CTimerList *CTimerUnit::GetTimerListByMSeconds(int to) {
	CTimerList *tl;

	for(tl = next; tl; tl=tl->next) {
		if(tl->timeout == to)
			return tl;
	}
	tl = new CTimerList(to);
	tl->next = next;
	next = tl;
	return tl;
}

CTimerUnit::CTimerUnit(void) : pending(0), next(NULL)
{
};

CTimerUnit::~CTimerUnit(void) {
	while(next) {
		CTimerList *tl = next;
		next = tl->next;
		delete tl;
	}
};

int CTimerUnit::ExpireMicroSeconds(int msec, int msec0) {
	int64_t exp;
	int64_t now;
	CTimerList *tl;

	now = GET_TIMESTAMP();
	exp = now + msec*(TIMESTAMP_PRECISION/1000);

	for(tl = next; tl; tl=tl->next) {
		if(tl->tlist.ListEmpty())
			continue;
		CTimerObject *o = tl->tlist.NextOwner();
		if(o->objexp < exp)
			exp = o->objexp;
	}
	
	exp -= now;
	if(exp <= 0)
		return 0;
	msec = exp / (TIMESTAMP_PRECISION/1000);
	return msec >= msec0 ? msec : msec0;
}

int CTimerUnit::CheckReady(void) {
	int n = 0;
	while(!pending.tlist.ListEmpty()) {
		CTimerObject *tobj = pending.tlist.NextOwner();
		tobj->ListDel();
		tobj->TimerNotify();
		n++;
	}
	return n;
}

int CTimerUnit::CheckExpired(int64_t now) {
	if(now==0)
		now = GET_TIMESTAMP();

	int n = CheckReady();;
	CTimerList *tl;
	for(tl = next; tl; tl=tl->next) {
		n += tl->CheckExpired(now);
	}
	return n;
}

void CReadyObject::ReadyNotify(uint64_t now)
{
        delete this;
}

int CReadyUnit::CheckReady(uint64_t now)
{
    int n = 0;
    CListObject<CReadyObject> * tmp;

    tmp = pending;
    pending = processing;
    processing = tmp;

    while(!processing->ListEmpty())
    {
	CReadyObject * robj = processing->NextOwner();
	robj->ListDel();
	robj->ReadyNotify(now);
	n++;
    }
    return n;
}
