#ifndef __TASK_REQUEST_PENDINGLIST_H
#define __TASK_REQUEST_PENDINGLIST_H

#include "timerlist.h"
#include "namespace.h"
#include "task_request.h"
#include <list>

TTC_BEGIN_NAMESPACE
/*
 * 请求挂起列表。
 *
 * 如果发现请求暂时没法满足，则挂起，直到
 *     1. 超时
 *     2. 条件满足被唤醒
 */
class CCacheProcess;
class CCacheBase;
class CTaskReqeust;
class CTimerObject;
class CTaskPendingList : private CTimerObject
{
    public:
        CTaskPendingList(CTaskDispatcher<CTaskRequest> *o, int timeout=5); 
        ~CTaskPendingList();

        void Add2List(CTaskRequest *);      //加入pending list
        void Wakeup(void);                  //唤醒队列中的所有task

    private:
        virtual void TimerNotify(void);

    private:
        CTaskPendingList(const CTaskPendingList &);
        const CTaskPendingList& operator=(const CTaskPendingList &);

    private:
        int               _timeout;
        CTimerList      * _timelist;
        CTaskDispatcher<CTaskRequest> * _owner;
        int	          _wakeup;
	typedef std::pair<CTaskRequest *, time_t> slot_t;
        std::list<slot_t> _pendlist;
};

TTC_END_NAMESPACE

#endif
