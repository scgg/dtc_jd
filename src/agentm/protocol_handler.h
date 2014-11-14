// Author: yizhiliu
// Date: 

#ifndef ZDB_PROTOCOL_HANDLER_H
#define ZDB_PROTOCOL_HANDLER_H

#include <queue>

#include "event_handler_impl.h"
#include "xbuffer.h"

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}

class ProtocolHandler;

struct MsgDesc
{
    int cmd;
    google::protobuf::Message *msg;
};

struct IProtocolProcessor
{
    virtual int HandleMessage(ProtocolHandler *me,
            uint32_t type, google::protobuf::Message *msg) = 0;
    virtual ~IProtocolProcessor() {}
};

class ProtocolHandler : public event_handler_impl
{
public:
    ProtocolHandler(int fd, epoll_reactor *reactor,
            IProtocolProcessor *processor);
    virtual ~ProtocolHandler();

    int handle_input(epoll_reactor *reactor);
    int handle_output(epoll_reactor *reactor);

    void QueueMessage(int type, google::protobuf::Message *msg);

private:
    IProtocolProcessor *m_processor;
    std::queue<MsgDesc> m_queuedMessage;
    xbuffer m_sendBuf, m_recvBuf;

    void KickOutput();
    void SerializeToSendBuf(MsgDesc &desc);
};

#endif

