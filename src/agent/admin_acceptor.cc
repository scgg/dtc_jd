#include <netinet/tcp.h>
#include <errno.h>

#include "log.h"
#include "poll_thread.h"
#include "admin_acceptor.h"
#include "admin_worker.h"


extern CBussinessModuleMgr *gBusinessModuleMgr;

int CAdminAcceptor::Build(const std::string &addr, CPollThread *thread, CPollThreadGroup *threadgroup,CConfig* gConfigObj,CFdDispacher *gFdDispacher)
{
    const char * errmsg = NULL;

    ownerThread = thread;
    m_threadgroup=threadgroup;
    m_gConfigObj=gConfigObj;
    m_gFdDispacher=gFdDispacher;
    errmsg = bindAddr.SetAddress(addr.c_str(), (const char *)NULL);
    if(errmsg)
    {
        log_error("%s %s", errmsg, addr.c_str());
        return -1;
    }

    netfd = bindAddr.CreateSocket();
    if(netfd < 0)
    {
        log_error("acceptor create socket failed: %m, %d", errno);
        return -1;
    }

    int optval = 1;
    setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
    optval = 60;
    setsockopt(netfd, SOL_TCP, TCP_DEFER_ACCEPT, &optval, sizeof(optval));

    if(bindAddr.BindSocket(netfd) < 0)
    {
        close(netfd);
        netfd = -1;
        log_error("acceptor bind address failed: %m, %d", errno);
        return -1;
    }

    if(bindAddr.SocketType() == SOCK_STREAM && listen(netfd, 255) < 0)
    {
        close(netfd);
        netfd = -1;
        log_error("acceptor listen failed: %m, %d", errno);
        return -1;
    }

    EnableInput();

    if(AttachPoller(ownerThread) < 0)
    {
        close(netfd);
        netfd = -1;
        log_error("acceptor attach listen socket failed");
        return -1;
    }

    return 0;
}

void CAdminAcceptor::InputNotify()
{
    int newfd;
    struct sockaddr peer;
    socklen_t peerSize = sizeof(peer);

    newfd = accept(netfd, (struct sockaddr *)&peer, &peerSize);
    if(newfd < 0)
    {
        if(errno != EINTR && errno != EAGAIN)
            log_notice("accept new client error: %m, %d", errno);
        return;
    }

    CAdminWorker *worker = new CAdminWorker(newfd, ownerThread, gBusinessModuleMgr,m_threadgroup,m_gConfigObj,m_gFdDispacher);

    if(NULL == worker)
    {
        log_error("no mem new CAGFrontWorker");
        return;
    }

    worker->InputNotify();
}
