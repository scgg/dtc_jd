#include "cache_flush.h"
#include "cache_process.h"
#include "global.h"

CFlushRequest::CFlushRequest(CCacheProcess *o, const char *key) :
	owner(o),
	numReq(0),
	badReq(0),
	wait(NULL)
{
}

CFlushRequest::~CFlushRequest()
{
	if(wait) {
		wait->ReplyNotify();
		wait = NULL;
	}
}

class CDropDataReply : public CReplyDispatcher<CTaskRequest> {
public:
	CDropDataReply(){}
	virtual void ReplyNotify(CTaskRequest *cur);
};

void CDropDataReply::ReplyNotify(CTaskRequest *cur)
{
	CFlushRequest *req = cur->OwnerInfo<CFlushRequest>();
	if(req==NULL)
		delete cur;
	else
		req->CompleteRow(cur, cur->OwnerIndex());
}

static CDropDataReply dropReply;

int CFlushRequest::FlushRow(const CRowValue &row)
{
	CTaskRequest* pTask = new CTaskRequest;
	if(pTask==NULL) {
		log_error("cannot flush row, new task error, possible memory exhausted\n");
		return -1;
	}
	
	if(pTask->Copy(row) < 0) {
		log_error("cannot flush row, from: %s   error: %s \n",
				pTask->resultInfo.ErrorFrom(),
				pTask->resultInfo.ErrorMessage()
				);
		return -1;
	}
	pTask->SetRequestType(TaskTypeCommit);
	pTask->PushReplyDispatcher(&dropReply);
	pTask->SetOwnerInfo(this, numReq, NULL);
	owner->IncAsyncFlushStat();
	//TaskTypeCommit never expired
	//pTask->SetExpireTime(3600*1000/*ms*/);
	numReq++;
	owner->PushFlushQueue(pTask);
	return 0;
}

void CFlushRequest::CompleteRow(CTaskRequest *req, int index)
{
	delete req;
	numReq--;
	if(numReq==0)
	{
	    if(wait) {
		wait->ReplyNotify();
		wait = NULL;
	    }
	    owner->CompleteFlushRequest(this);
	}
}

MARKER_STAMP CCacheProcess::CalculateCurrentMarker()
{
	time_t now;

	time(&now);
	return now - (now % markerInterval);
}

void CCacheProcess::SetDropCount(int c)
{
//	Cache.set_drop_count(c);
}


void CCacheProcess::GetDirtyStat()
{
	uint64_t ullMaxNode;
	uint64_t ullMaxRow;
	const double rate = 0.9;
	
	if(CBinMalloc::Instance()->UserAllocSize() >= CBinMalloc::Instance()->TotalSize()*rate){
		ullMaxNode = Cache.TotalUsedNode();
		ullMaxRow = Cache.TotalUsedRow();
	}
	else{
		if(CBinMalloc::Instance()->UserAllocSize() > 0){
			double enlarge = CBinMalloc::Instance()->TotalSize()*rate / CBinMalloc::Instance()->UserAllocSize();
			ullMaxNode = (uint64_t)(Cache.TotalUsedNode() * enlarge);
			ullMaxRow = (uint64_t)(Cache.TotalUsedRow() * enlarge);
		}
		else{
			ullMaxNode = 0;
			ullMaxRow = 0;
		}
	}
	
}


void CCacheProcess::SetFlushParameter(
	int intvl,
	int mreq,
	int mintime,
	int maxtime
)
{
	// require v4 cache
	if(Cache.GetCacheInfo()->version < 4)
		return;

	/*
	if(intvl < 60)
		intvl = 60;
	else if(intvl > 43200)
		intvl = 43200;
	*/
	
	/* marker time interval changed to 1sec */
	intvl = 1;
	markerInterval = intvl;
	
	/* 1. make sure at least one time marker exist
	 * 2. init first marker time and last marker time
	 * */
	CNode stTimeNode = Cache.FirstTimeMarker();
	if(!stTimeNode)
		Cache.InsertTimeMarker(CalculateCurrentMarker());
	Cache.FirstTimeMarkerTime();
	Cache.LastTimeMarkerTime();

	if(mreq <= 0)
		mreq = 1;
	if(mreq > 10000)
		mreq = 10000;

	if(mintime < 10)
		mintime = 10;
	if(maxtime <= mintime)
		maxtime = mintime * 2;

	maxFlushReq = mreq;
	minDirtyTime = mintime;
	maxDirtyTime = maxtime;
	
	//GetDirtyStat();

	/*attach timer only if async mode or sync mode but mem dirty*/
	if(updateMode == MODE_ASYNC ||
			(updateMode == MODE_SYNC && mem_dirty == true))
	{
		/* check for expired dirty node every second */
		flushTimer = owner->GetTimerList(1);
		AttachTimer(flushTimer);
	}
}

int CCacheProcess::CommitFlushRequest(CFlushRequest *req, CTaskRequest* callbackTask)
{
	req->wait = callbackTask;
	
	if(req->numReq==0)
		delete req;
	else
		nFlushReq++;

	statCurrFlushReq = nFlushReq;
	return 0;
}

void CCacheProcess::CompleteFlushRequest(CFlushRequest *req)
{
	delete req;
	nFlushReq --;
	statCurrFlushReq = nFlushReq;

	CalculateFlushSpeed(0);

	if(nFlushReq < mFlushReq)
		FlushNextNode();
}

void CCacheProcess::TimerNotify(void)
{
	log_debug("flush timer event...");
	int ret = 0;
	
	MARKER_STAMP cur = CalculateCurrentMarker();
	if(Cache.FirstTimeMarkerTime() != cur)
		Cache.InsertTimeMarker(cur);

	CalculateFlushSpeed(1);

	/* flush next node return
	 * 1: no dirty node exist, sync ttc, should not attach timer again
	 * 0: one flush request created, nFlushReq inc in FlushNextNode, notinue
	 * others: on flush request created due to some reason, should break for another flush timer event, otherwise may be    
	 * block here, eg. no dirty node exist, and in async mode
	 * */
	while(nFlushReq < mFlushReq)
	{
	    ret = FlushNextNode();
	    if(ret == 0)
	    {
		continue;
	    }else
	    {
		break;
	    }
	}

	/*SYNC + mem_dirty/ASYNC need to reattach flush timer*/
	if((updateMode == MODE_SYNC && mem_dirty == true)
			|| updateMode == MODE_ASYNC)
		AttachTimer(flushTimer);
}

int CCacheProcess::OldestDirtyNodeAlarm()
{
	CNode stHead = Cache.DirtyLruHead();
	CNode stNode = stHead.Prev();
	
	if(Cache.IsTimeMarker(stNode))
	{
		stNode = stNode.Prev();
		if(Cache.IsTimeMarker(stNode) || stNode == stHead)
		{
			return 0;
		}else
		{
			return 1;
		}
	}else if(stNode == stHead)
	{
		return 0;
	}else
	{
		return 1;
	}
}


/*flush speed(nFlushReq) only depend on oldest dirty node existing time -- newman*/
void CCacheProcess::CalculateFlushSpeed(int is_flush_timer)
{
	DeleteTailTimeMarkers();

	// time base
	int m, v;
	unsigned int t1 = Cache.FirstTimeMarkerTime();
	unsigned int t2 = Cache.LastTimeMarkerTime();
	//initialized t1 and t2, so no need of test for this
	v =  t1 - t2;

	//if start with sync and mem dirty, flush as fast as we can
	if(updateMode == MODE_SYNC)
	{
		if(mem_dirty == false)
		{
			mFlushReq = 0;
		}else
		{
			mFlushReq = maxFlushReq;
		}
		goto __stat;
	}

	//alarm if oldest dirty node exist too much time, flush at fastest speed
	if(v >= maxDirtyTime)
	{
		mFlushReq = maxFlushReq;
		if(OldestDirtyNodeAlarm() && is_flush_timer)
		{
			log_notice("oldest dirty node exist time > max dirty time");
		}
	}else if(v >= minDirtyTime)
	{
		m = 1 + (v - minDirtyTime) * (maxFlushReq-1) / (maxDirtyTime - minDirtyTime);
		if(m > mFlushReq)
			mFlushReq = m;
	}else
	{
		mFlushReq = 0;
	}

__stat:
	if(mFlushReq > maxFlushReq)
		mFlushReq = maxFlushReq;

	statMaxFlushReq = mFlushReq;
	statOldestDirtyTime = v;
}

/* return -1: encount the only time marker
 * return  1: no dirty node exist, clear mem dirty
 * return  2: no dirty node exist, in async mode
 * return -2: no flush request created
 * return  0: one flush request created
 * */
int CCacheProcess::FlushNextNode(void)
{
    unsigned int uiFlushRowsCnt = 0;
    MARKER_STAMP stamp;
    static MARKER_STAMP last_rm_stamp;

    CNode stHead = Cache.DirtyLruHead();
    CNode stNode = stHead;
    CNode stPreNode = stNode.Prev();

    /*case 1: delete continues time marker, until 
     *        encount a normal node/head node, go next
     *        encount the only time marker*/
    while(1)
    {
        stNode = stPreNode;
        stPreNode = stNode.Prev();

        if(!Cache.IsTimeMarker(stNode))
            break;

        if(Cache.FirstTimeMarkerTime() == stNode.Time())
        {
	    if(updateMode == MODE_SYNC && mem_dirty == true)
	    {
		/* delete this time marker, flush all dirty node */
		Cache.RemoveTimeMarker(stNode);
		stNode = stPreNode;
		stPreNode = stNode.Prev();
		while(stNode != stHead)
                {
			cache_flush_data_timer(stNode, uiFlushRowsCnt);
			stNode = stPreNode;
			stPreNode = stNode.Prev();
		}
		
		DisableTimer();
		mem_dirty = false;
		log_notice("mem clean now for sync cache");
		return 1;
	    }
	    return -1;
        }

        stamp = stNode.Time();
        if(stamp > last_rm_stamp)
        {
            last_rm_stamp = stamp;
        }

	log_debug("remove time marker in dirty lru, time %u", stNode.Time());
        Cache.RemoveTimeMarker(stNode);
    }

    /*case 2: this the head node, clear mem dirty if nessary, return, should not happen*/
    if(stNode == stHead)
    {
        if(updateMode==MODE_SYNC && mem_dirty == true)
        {
	    DisableTimer();
            mem_dirty = false;
            log_notice("mem clean now for sync cache");
            return 1;
        }else
        {
            return 2;
        }
    }

    /*case 3: this a normal node, flush it.
     * 	  return -2 if no flush request added to cache process
     * */
    int iRet = cache_flush_data_timer(stNode, uiFlushRowsCnt);
    if(iRet == -1 || iRet == -2 || iRet == -3 || iRet == 1)
    {
        return -2;
    }

    return 0;
}

void CCacheProcess::DeleteTailTimeMarkers()
{
    CNode stHead = Cache.DirtyLruHead();
    CNode stNode = stHead;
    CNode stPreNode = stNode.Prev();

    while(1)
    {
        stNode = stPreNode;
        stPreNode = stNode.Prev();

        if(stNode == stHead || Cache.FirstTimeMarkerTime() == stNode.Time())
            break;

        if(Cache.IsTimeMarker(stNode) && Cache.IsTimeMarker(stPreNode))
            Cache.RemoveTimeMarker(stNode);
        else
            break;
    }
}
