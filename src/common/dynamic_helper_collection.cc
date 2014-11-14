#include "helper_group.h"
#include "StatTTCDef.h"

#include "dynamic_helper_collection.h"

CDynamicHelperCollection::CDynamicHelperCollection(CPollThread *owner, int clientPerGroup):
    CTaskDispatcher<CTaskRequest>(owner),
    m_recvTimerList(0), m_connTimerList(0), m_retryTimerList(0), m_clientPerGroup(clientPerGroup)
{
}

CDynamicHelperCollection::~CDynamicHelperCollection()
{
    for(HelperMapType::iterator iter = m_groups.begin();
            iter != m_groups.end(); ++iter)
    {
        delete iter->second.group;
    }

    m_groups.clear();
}

void CDynamicHelperCollection::TaskNotify(CTaskRequest *t)
{
    log_debug("CDynamicHelperCollection::TaskNotify start, t->RemoteAddr: %s", t->RemoteAddr());
    HelperMapType::iterator iter = m_groups.find(t->RemoteAddr());
    if(iter == m_groups.end())
    {
        CHelperGroup *g = new CHelperGroup(
                t->RemoteAddr(), /* sock path */
                t->RemoteAddr(), /* name */
                m_clientPerGroup, /* helper client count */
                m_clientPerGroup * 32 /* queue size */,
                TTC_FORWARD_USEC_ALL);
        g->SetTimerHandler(m_recvTimerList, m_connTimerList, m_retryTimerList);
        g->Attach(owner, &m_taskQueueAllocator);
        helper_group hg = { g, 0 };
        m_groups[t->RemoteAddr()] = hg;
        iter = m_groups.find(t->RemoteAddr());
    }
    t->PushReplyDispatcher(this);
    iter->second.group->TaskNotify(t);
    iter->second.used = 1;
   log_debug("CDynamicHelperCollection::TaskNotify end");
}

void CDynamicHelperCollection::ReplyNotify(CTaskRequest *t)
{
	if(t->ResultCode() == 0)
	{
		log_debug ("reply from remote ttc success,append result start ");

        if(t->result)
        {
            t->PrepareResult();
            int iRet = t->PassAllResult(t->result);
            if(iRet < 0) {
                log_notice("task append_result error: %d", iRet);
                t->SetError(iRet, "CDynamicHelperCollection", "AppendResult() error");
                t->ReplyNotify();
                return;
            }
        }
		t->ReplyNotify();
		return;
	}
	else
	{
		log_debug ("reply from remote ttc error:%d",t->ResultCode());
		t->ReplyNotify();
		return;
	}

}

void CDynamicHelperCollection::SetTimerHandler(CTimerList *recv,
        CTimerList *conn, CTimerList *retry)
{
    m_recvTimerList = recv;
    m_connTimerList = conn;
    m_retryTimerList = retry;

    AttachTimer(m_retryTimerList);
}

void CDynamicHelperCollection::TimerNotify()
{
    for(HelperMapType::iterator i = m_groups.begin(); i != m_groups.end();)
    {
        if(i->second.used == 0)
        {
            delete i->second.group;
            m_groups.erase(i++);
        }
        else
        {
            i->second.used = 0;
            ++i;
        }
    }
}


