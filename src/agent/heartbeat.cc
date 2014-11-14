#include <google/protobuf/message.h>

#include "log.h"
#include "admin.h"
#include "sockaddr.h"
#include "poll_thread.h"
#include "admin_protocol.pb.h"
#include "config_file.h"

#include "heartbeat.h"

CAgentHeartBeatWorker::CAgentHeartBeatWorker(uint32_t agentId, const char *masterAddr, CPollThread *thread)
{
    m_masterAddr = masterAddr;
    m_ownerThread = thread;
    m_sender = NULL;
    m_status = HS_DOWN;
    m_downloading = false;
    m_timerList = thread->GetTimerList(5);
	m_agentId = agentId;
    AttachTimer(m_timerList);
}

CAgentHeartBeatWorker::~CAgentHeartBeatWorker()
{
    delete m_sender;
}

int CAgentHeartBeatWorker::Build()
{
    CSocketAddress addr;
    const char *err = addr.SetAddress(m_masterAddr.c_str(), (const char *)NULL);
    if(err)
    {
        log_error("invalid master address");
        return -1;
    }

    if(netfd > 0)
        close(netfd);
    netfd = addr.CreateSocket();
    if(netfd < 0)
    {
        log_error("create socket error: %d, %m", errno);
        return -1;
    }

    struct timeval tv = { 0, 50000 }; //50ms
    setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    m_status = HS_CONNECTING;
    if(addr.ConnectSocket(netfd) < 0)
    {
        log_error("connect master error: %d, %m", errno);
        close(netfd);
        netfd = -1;
        m_status = HS_DOWN;
        return -1;
    }

    EnableInput();
    DisableOutput();
    //if(AttachPoller(m_ownerThread) < 0)
    if(AttachPoller(ownerUnit ? NULL : m_ownerThread) < 0)
    {
        log_error("attach poller faild");
        close(netfd);
        netfd = -1;
        m_status = HS_DOWN;
        return -1;
    }


    m_sender = new CAGSender(netfd, 1024);
    m_sender->Build();
    m_status = HS_UP;
    DelayApplyEvents();
    return 0;
}

void CAgentHeartBeatWorker::Exit()
{
    DetachPoller();
//    DisableTimer();
    close(netfd);
    netfd = -1;
    delete m_sender;
    m_sender = NULL;
    m_status = HS_DOWN;
    m_downloading = false;
    m_recvBuf.reset();
}

void CAgentHeartBeatWorker::HangupNotify()
{
    Exit();
}

void CAgentHeartBeatWorker::InputNotify()
{
    int rtn = m_recvBuf.readfrom(netfd);
    if(rtn < 0)
    {
        log_error("recv error : %m");
        Exit();
        return;  //don't notify me more
    }

    if(rtn == 0)
    {
        Exit();
        return;
    }

    if(m_recvBuf.len() < sizeof(ProtocolHeader))
        return;

    ProtocolHeader *header = (ProtocolHeader *)m_recvBuf.readpos();

    if(!IsProtocolHeaderValid(header))
    {
        log_error("invalid packet header recved, %x, %x, %x",
                header->magic, header->length, header->cmd);
        Exit();
        return;
    }

    while(header->length + sizeof(ProtocolHeader) <= m_recvBuf.len())
    {
        m_recvBuf.consume(sizeof(ProtocolHeader));
        //ok, we got a full packet
        google::protobuf::Message *message = CreateMessage(header->cmd);
        if(!message)
        {
            Exit();
            return;
        }
        if(!message->ParseFromArray(m_recvBuf.readpos(),
                    header->length))
        {
            delete message;
            log_error("parse message error");
            Exit();
            return;
        }

        m_recvBuf.consume(header->length);

        if(HandleMessage(header->cmd, message) < 0)
        {
            log_error("handle message error");
            Exit();
            return;
        }

        if(m_recvBuf.len() < sizeof(ProtocolHeader))
        {
            m_recvBuf.tidy();
            return;
        }

        header = (ProtocolHeader *)m_recvBuf.readpos();
        if(!IsProtocolHeaderValid(header))
        {
            log_error("invalid packet header recved, %x, %x, %x",
                    header->magic, header->length, header->cmd);
            Exit();
            return;
        }
    }

    m_recvBuf.tidy();
    m_recvBuf.ensure_capacity(header->length + sizeof(ProtocolHeader));

    return;
}

int CAgentHeartBeatWorker::HandleMessage(int type,
        google::protobuf::Message *message)
{
    if(type == AC_HeartBeatReply)
    {
        ttc::agent::HeartBeatReply *reply =
            (ttc::agent::HeartBeatReply *)message;

        log_debug("HeartBeatReply version[%d], cksum[%s], push[%s]",
                reply->latest_configure_version(),
                reply->latest_configure_md5().c_str(),
                reply->push() ? "YES" : "NO");

        if(reply->push() &&
                reply->latest_configure_version() > gCurrentConfigureVersion)
        {
            if(reply->latest_configure_md5() == gCurrentConfigureCksum)
            {
                log_notice("cksum remains the same while version number changed!!!");
                delete message;
                return 0;
            }

            if(m_downloading)
            {
                delete message;
                return 0;
            }

            log_info("begin to get the latest configure ...");

            ttc::agent::GetLatestConfigure request;
            request.set_configure_version(reply->latest_configure_version());
            int byteSize = request.ByteSize();
            unsigned char *buf =
                (unsigned char *)malloc(sizeof(ProtocolHeader) + byteSize);
            ProtocolHeader *header = (ProtocolHeader *)buf;
            header->magic = ADMIN_PROTOCOL_MAGIC;
            header->cmd = AC_GetLatestConfigure;
            header->length = byteSize;
            request.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));
            m_sender->AddPacket((const char *)buf, byteSize + sizeof(ProtocolHeader));
            m_downloading = true;
            delete message;
            return 0;
        }
        else
        {
            delete message;
            return 0;
        }
    }
    else if(type == AC_LatestConfigureReply)
    {
        ttc::agent::LatestConfigureReply *reply =
            (ttc::agent::LatestConfigureReply *)message;
        std::string errorMessage;
        if(!IsConfigValid(reply->configure().c_str(), &errorMessage))
        {
            log_error("invalid config received: %s", errorMessage.c_str());
            log_error("config : \n%s", reply->configure().c_str());
            delete reply;
            m_downloading = false;
            return 0;
        }

        WriteConfig(reply->configure().c_str());
        log_info("get new configure: \n%s", reply->configure().c_str());
        log_info("restart now");
        delete message;
        exit(2);
        return 0;
    }
    else
    {
        delete message;
        return -1;
    }
}

google::protobuf::Message *CAgentHeartBeatWorker::CreateMessage(int type)
{
    switch(type)
    {
    case AC_HeartBeatReply:
        return new ttc::agent::HeartBeatReply;
    case AC_LatestConfigureReply:
        return new ttc::agent::LatestConfigureReply;
    default:
        log_error("got a unknown packet: %d", type);
        return NULL;
    }
}

void CAgentHeartBeatWorker::OutputNotify()
{
    if(m_sender->Send() < 0)
    {
        log_error("send error!");
        Exit();
        return;
    }

    m_sender->SendDone() ? DisableOutput() : EnableOutput();
    DelayApplyEvents();
}

extern const char *versionStr;

void CAgentHeartBeatWorker::TimerNotify()
{
#ifdef DEBUG
    log_info("enter TimerNotify.");
#endif
    AttachTimer(m_timerList);

    if(m_status == HS_DOWN)
        Build();

    if(!m_sender)
        return;

    ttc::agent::HeartBeat heartbeat;
    heartbeat.set_version(versionStr);
    heartbeat.set_configure_version(gCurrentConfigureVersion);
    heartbeat.set_configure_md5(gCurrentConfigureCksum);
	heartbeat.set_agent_id(m_agentId);

    int byteSize = heartbeat.ByteSize();
    unsigned char *buf = (unsigned char *)malloc(byteSize + sizeof(ProtocolHeader));
    ProtocolHeader *header = (ProtocolHeader *)buf;
    header->magic = ADMIN_PROTOCOL_MAGIC;
    header->length = byteSize;
    header->cmd = AC_HeartBeat;
    heartbeat.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));

#ifdef DEBUG
    log_info("ready to send msg");
#endif

    m_sender->AddPacket((const char *)buf, byteSize + sizeof(ProtocolHeader));
    OutputNotify();
}
