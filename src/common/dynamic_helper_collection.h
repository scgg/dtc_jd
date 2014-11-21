#ifndef DYNAMIC_HELPER_COLLECTION_H__
#define DYNAMIC_HELPER_COLLECTION_H__

#include <map>
#include <string>
#include "task_request.h"

class CHelperGroup;
class CPollThread;
class CTimerList;

class CDynamicHelperCollection : public CTaskDispatcher<CTaskRequest>,
    public CReplyDispatcher<CTaskRequest>, public CTimerObject
{
public:
    CDynamicHelperCollection(CPollThread *owner, int clientPerGroup);
    ~CDynamicHelperCollection();

    void SetTimerHandler(CTimerList *recv, CTimerList *conn,
            CTimerList *retry);

private:
    virtual void TaskNotify(CTaskRequest *t);
    virtual void ReplyNotify(CTaskRequest *t);
    virtual void TimerNotify();

    struct helper_group
    {
        CHelperGroup *group;
        int used;
    };

    typedef std::map<std::string, helper_group> HelperMapType;
    HelperMapType m_groups;
    CLinkQueue<CTaskRequest *>::allocator m_taskQueueAllocator;
    CTimerList *m_recvTimerList, *m_connTimerList, *m_retryTimerList;
    int m_clientPerGroup;
};

#endif
