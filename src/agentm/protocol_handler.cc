#include <google/protobuf/message.h>

#include "admin.h"
#include "protocol_handler.h"
#include "message_builder.h"
#include "log.h"

ProtocolHandler::ProtocolHandler(int fd, epoll_reactor *reactor,
        IProtocolProcessor *processor) :
    event_handler_impl(reactor, fd, EPOLLIN), m_processor(processor),
    m_sendBuf(16*1024), m_recvBuf(16*1024)
{
    enable_input_notify();
}

ProtocolHandler::~ProtocolHandler()
{
    delete m_processor;
    while(!m_queuedMessage.empty())
    {
        delete m_queuedMessage.front().msg;
        m_queuedMessage.pop();
    }
}

int ProtocolHandler::handle_input(epoll_reactor *reactor)
{
    int rtn = m_recvBuf.readfrom(fd());
    if(rtn < 0)
    {
        log_error("recv error : %m");
        reactor->push_delay_task(delete_, this);
        return -1;  //don't notify me more
    }

    if(rtn == 0)
    {
        reactor->push_delay_task(delete_, this);
        return -1;
    }

    if(m_recvBuf.len() < sizeof(ProtocolHeader))
        return 0;

    ProtocolHeader *header = (ProtocolHeader *)m_recvBuf.readpos();
    if(!IsProtocolHeaderValid(header))
    {
        log_error("invalid packet header recved, %x, %x, %x",
                header->magic, header->cmd, header->length);
        reactor->push_delay_task(delete_, this);
        return -1;
    }

    while(header->length + sizeof(ProtocolHeader) <= m_recvBuf.len())
    {
        m_recvBuf.consume(sizeof(ProtocolHeader));

        //ok, we got a full packet
        google::protobuf::Message *message = CreateMessage(header->cmd);
        if(!message->ParseFromArray(m_recvBuf.readpos(),
                    header->length))
        {
            delete message;
            log_error("parse message error");
            reactor->push_delay_task(delete_, this);
            return -1;
        }

        if(m_processor->HandleMessage(this, header->cmd, message) < 0)
        {
            log_error("handle message error");
            reactor->push_delay_task(delete_, this);
            return -1;
        }
        
        m_recvBuf.consume(header->length);

        if(m_recvBuf.len() < sizeof(ProtocolHeader))
        {
            m_recvBuf.tidy();
            return 0;
        }

        header = (ProtocolHeader *)m_recvBuf.readpos();
        if(!IsProtocolHeaderValid(header))
        {
            log_error("invalid packet header recved, %x, %x, %x",
                    header->magic, header->cmd, header->length);
            reactor->push_delay_task(delete_, this);
            return -1;
        }

    }

    m_recvBuf.tidy();
    m_recvBuf.ensure_capacity(header->length + sizeof(ProtocolHeader));

    return 0;
}

void ProtocolHandler::QueueMessage(int cmd, google::protobuf::Message *msg)
{
    MsgDesc desc = { cmd, msg };
    m_queuedMessage.push(desc);
    if(m_queuedMessage.size() == 1 && m_sendBuf.len() == 0)
        KickOutput();
}

void ProtocolHandler::SerializeToSendBuf(MsgDesc &desc)
{
    int bytes = desc.msg->ByteSize();
    m_sendBuf.ensure_capacity(bytes + sizeof(ProtocolHeader));
    ProtocolHeader *header = (ProtocolHeader *)m_sendBuf.writepos();
    header->magic = ADMIN_PROTOCOL_MAGIC;
    header->cmd = desc.cmd;
    header->length = bytes;
    m_sendBuf.gotbytes(sizeof(ProtocolHeader));
    desc.msg->SerializeWithCachedSizesToArray((unsigned char *)m_sendBuf.writepos());
    m_sendBuf.gotbytes(bytes);
}

void ProtocolHandler::KickOutput()
{
    if(m_sendBuf.len() > 0 || m_queuedMessage.empty())
        return;

    MsgDesc &desc = m_queuedMessage.front();
    SerializeToSendBuf(desc);
    delete desc.msg;
    m_queuedMessage.pop();

    int rtn = m_sendBuf.writeto(fd());
    if(rtn < 0)
    {
        log_error("send error, %m");
        reactor()->push_delay_task(delete_, this);
        return;
    }

    if(m_sendBuf.len() > 0)     //we have things in sendbuf
        enable_output_notify();
}

int ProtocolHandler::handle_output(epoll_reactor *reactor)
{
    while(m_sendBuf.len() > 0 || !m_queuedMessage.empty())
    {
        while(m_sendBuf.len() < 4096 && !m_queuedMessage.empty())
        {
            MsgDesc &desc = m_queuedMessage.front();
            SerializeToSendBuf(desc);
            delete desc.msg;
            m_queuedMessage.pop();
        }

        int rtn = m_sendBuf.writeto(fd());
        if(rtn < 0)
        {
            log_error("send error, %m");
            reactor->push_delay_task(delete_, this);
            return -1;
        }

        if(m_sendBuf.len() > 0)
            return 0;
    }

    if(m_sendBuf.len() == 0 && m_queuedMessage.empty())
        disable_output_notify();

    return 0;
}

