#ifndef ADMIN_ACCEPTOR_H__
#define ADMIN_ACCEPTOR_H__

#include "poller.h"
#include "sockaddr.h"
#include "poll_thread_group.h"
#include "CFdDispacher.h"

class CPollThread;
class CTimerList;

class CAdminAcceptor: public CPollerObject
{
public:
    CAdminAcceptor() : ownerThread(0), idleList(0),m_threadgroup(0) {}

    int Build(const std::string &addr, CPollThread *thread,CPollThreadGroup *threadgroup,CConfig* gConfigObj,CFdDispacher *gFdDispacher);
    virtual void InputNotify();

private:
    CSocketAddress bindAddr;
    CPollThread *ownerThread;
    CTimerList *idleList;
    CPollThreadGroup *m_threadgroup;
    CConfig* m_gConfigObj;
    CFdDispacher* m_gFdDispacher;

};

#endif
