#include "task_pendlist.h"
#include "cache_process.h"
#include "log.h"

TTC_USING_NAMESPACE

CTaskPendingList::CTaskPendingList(CTaskDispatcher<CTaskRequest> *o, int to):
    _timeout(to),
    _timelist(0),
    _owner(o),
    _wakeup(0)
{
    _timelist= _owner->owner->GetTimerList(_timeout);
}

CTaskPendingList::~CTaskPendingList()
{
    std::list<slot_t>::iterator it;
    for(it = _pendlist.begin(); it != _pendlist.end(); ++it)
    {
        //把所有请求踢回客户端
        it->first->SetError(-ETIMEDOUT, __FUNCTION__,  "object deconstruct");
        it->first->ReplyNotify();
    }
}

void CTaskPendingList::Add2List(CTaskRequest *task)
{

    if(task)
    {
        if(_pendlist.empty())
            AttachTimer(_timelist);

	_pendlist.push_back(std::make_pair(task, time(NULL)));
    }

    return;
}

// 唤醒队列中所有已经pending的task
void CTaskPendingList::Wakeup(void)
{

    log_debug("CTaskPendingList Wakeup");

    //唤醒所有task
    _wakeup = 1;

    AttachReadyTimer(_owner->owner);

    return;
}


void CTaskPendingList::TimerNotify(void)
{

    std::list<slot_t> copy;
    copy.swap(_pendlist);
    std::list<slot_t>::iterator it;

    if(_wakeup)
    {
        for(it = copy.begin(); it != copy.end(); ++it)
        {
            _owner->TaskNotify(it->first);
        }

        _wakeup = 0;
    }
    else
    {

        time_t now = time(NULL);

        for(it = copy.begin(); it != copy.end(); ++it)
        {
            //超时处理
            if(it->second + _timeout >= now) {
		    _pendlist.push_back(*it);
	    } else {
		    it->first->SetError(-ETIMEDOUT, __FUNCTION__, "pending task is timedout");
		    it->first->ReplyNotify();
	    }
        }

        if(!_pendlist.empty())
            AttachTimer(_timelist);
    }
    
    return;
}
