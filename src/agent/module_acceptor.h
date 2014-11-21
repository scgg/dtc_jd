/*
 * =====================================================================================
 *
 *       Filename:  module_accepter.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/01/2010 06:06:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef AG_MODULE_ACCEPTER_H__
#define AG_MODULE_ACCEPTER_H__

#include <string>

#include "poller.h"
#include "sockaddr.h"
#include "module.h"

class CPollThread;
class CTimerList;

class CModuleAcceptor: public CPollerObject,public CTimerObject
{
public:
    CModuleAcceptor() : ownerThread(0), idleList(0), ownerModule(0),threadgroup(0),threadid(0) {}

    int Build(const std::string &addr, CPollThread *thread, CModule *owner,CPollThreadGroup *threadgroup);
    virtual void InputNotify();
    void CloseListenFd();

private:
    CSocketAddress bindAddr;
    CPollThread *ownerThread;
    CTimerList *idleList;
    CModule *ownerModule;
    CPollThreadGroup *threadgroup;
    int threadid;
};

#endif
