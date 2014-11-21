// Author: yizhiliu
// Date: 

#ifndef HEARTBEAT_H__
#define HEARTBEAT_H__

#include <string>

#include "poller.h"
#include "timerlist.h"
#include "sender.h"
#include "xbuffer.h"

class CPollThread;

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}

class CAgentHeartBeatWorker : public CPollerObject, public CTimerObject
{
public:
    CAgentHeartBeatWorker(uint32_t agentId, const char *masterAddr, CPollThread *ownerThread);
    ~CAgentHeartBeatWorker();

    virtual void InputNotify();
    virtual void OutputNotify();
    virtual void TimerNotify();
    virtual void HangupNotify();

    int Build();
    void Exit();
private:
    int HandleMessage(int type, google::protobuf::Message *message);
    google::protobuf::Message *CreateMessage(int type);
    std::string m_masterAddr;
    CPollThread *m_ownerThread;
    CTimerList *m_timerList;
    CAGSender *m_sender;
    xbuffer m_recvBuf;
    bool m_downloading;

    enum HealthStatus
    {
        HS_DOWN,
        HS_CONNECTING,
        HS_UP,
    };
    HealthStatus m_status;
	uint32_t     m_agentId;
};


#endif

