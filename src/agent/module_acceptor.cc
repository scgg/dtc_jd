/*
 * =====================================================================================
 *
 *       Filename:  module_acceptor.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/01/2010 06:18:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <netinet/tcp.h>
#include <errno.h>

#include "log.h"
#include "poll_thread.h"
#include "module_acceptor.h"
#include "front_worker.h"
#include "LockFreeQueue.h"

extern int gClientIdleTime;

int CModuleAcceptor::Build(const std::string &addr, CPollThread *thread, CModule *owner,CPollThreadGroup *threadgrp)
{
    const char * errmsg = NULL;

    ownerThread = thread;
    ownerModule = owner;
    threadgroup = threadgrp;
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

    idleList = ownerThread->GetTimerList(gClientIdleTime);
    if(NULL == idleList)
    {
        log_error("get idlelist from agent thread failed");
        return -1;
    }
    return 0;
}

void CModuleAcceptor::InputNotify()
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
    if((ownerModule->GetCurConnection())>4096)
    {
        close(newfd);
        return;
    }

    //对套接字开启keepalive选项
    int keepAlive = 1;   // 开启keepalive属性. 缺省值: 0(关闭)
    int keepIdle = 60;   // 如果在60秒内没有任何数据交互,则进行探测. 缺省值:7200(s)
    int keepInterval = 3;  // 探测时发探测包的时间间隔为3秒. 缺省值:75(s)
    int keepCount = 3;   // 探测重试的次数. 全部超时则认定连接失效..缺省值:9(次)
    setsockopt(newfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
    setsockopt(newfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(newfd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
    setsockopt(newfd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));
    log_debug("the fd put into queue is %d,threadid is %d",newfd,threadid);
    //log_error("size of queue is  %d",ownerModule->QueueCountFd(threadid));
    int ret=ownerModule->DispachAcceptFd(threadid,newfd);
    if(ret<0)
    {
    	close(newfd);
    	log_error("dispacher fd error,close the connection!");
    }
    threadid++;
    if(threadid == (threadgroup->GetPollThreadSize()))
    {
    	threadid=0;
    }
}

void CModuleAcceptor::CloseListenFd()
{
	close(netfd);
	netfd=0;
}
