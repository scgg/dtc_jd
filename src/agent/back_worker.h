/*
 * =====================================================================================
 *
 *       Filename:  back_worker.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2010 06:51:52 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef AG_BACK_WORKER_H__
#define AG_BACK_WORKER_H__

#include <string>

#include "poller.h"
#include "timerlist.h"
#include "sockaddr.h"
#include "lock.h"

class CPollThread;
class CModule;
class CAGSender;
class CAGReceiver;

class CBackWorker: public CPollerObject, public CTimerObject
{
public:
    CBackWorker() : m_status(HS_DOWN), m_ownerThread(0), m_ownerModule(0),
    m_sender(0), m_receiver(0)
    {
    }
    ~CBackWorker();

    int Build(const std::string &addrstr, CPollThread *thread, CModule *module);
    bool GetReady();
    int HandleRequest(const char *buf, int len);

private:
    enum HealthStatus
    {
        HS_DOWN,
        HS_CONNECTING,
        HS_UP,
    };
    HealthStatus m_status;

    CSocketAddress ttcAddr;
    CPollThread *m_ownerThread;
    CModule *m_ownerModule;

    CAGSender *m_sender;
    CAGReceiver *m_receiver;
    CMutex       m_lock;

private:
    virtual void InputNotify();
    virtual void OutputNotify();
    virtual void TimerNotify();
    virtual void HangupNotify() { Exit(); }

    int Recv();
    int Send();
    int ConnectTTC();
    void Exit();
};

#endif
