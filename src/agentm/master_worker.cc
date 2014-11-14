#include "admin.h"
#include "admin_protocol.pb.h"
#include "status_updater.h"
#include "configure_manager.h"
#include "log.h"
#include "master_worker.h"

int MasterWorker::HandleMessage(ProtocolHandler *me,
        uint32_t type, google::protobuf::Message *msg)
{
    if(type == AC_HeartBeat)
    {
        ttc::agent::HeartBeat *heartbeat = (ttc::agent::HeartBeat *)msg;
        m_statusUpdater->Update(m_remoteAddr.c_str(), heartbeat);
        delete heartbeat;
#ifdef DEBUG
        log_info("HearBeat msg.\n");
        if( m_confManager->PushSetted())
        {
            log_info("Push = 1");
        }
        else
        {
            log_info("Push = 0");
        }
#endif
        if (m_confManager->FetchConfigInfoFromDatabase() < 0)
        	log_error("FetchConfigInfoFromDatabase fail");

        ttc::agent::HeartBeatReply *reply = new ttc::agent::HeartBeatReply;
        reply->set_latest_configure_version(
                m_confManager->LatestConfigureVersion());
        reply->set_latest_configure_md5(
                m_confManager->LatestConfigureMd5());
        reply->set_push(m_confManager->PushSetted());

        me->QueueMessage(AC_HeartBeatReply, reply);
        return 0;
    }
    else if(type == AC_GetLatestConfigure)
    {
        delete msg;
#ifdef DEBUG
        log_info("GetLatestConfigure msg.\n");
#endif
        ttc::agent::LatestConfigureReply *reply = new ttc::agent::LatestConfigureReply;
        reply->set_configure_version(m_confManager->LatestConfigureVersion());
        reply->set_configure(m_confManager->LatestConfigure());
#ifdef DEBUG
        log_info("%s", m_confManager->LatestConfigure().c_str());
#endif

        me->QueueMessage(AC_LatestConfigureReply, reply);
        return 0;
    }
    else if(type == AC_UpdateConfigure)
    {
#ifdef DEBUG
        log_info("UpdateConfigure msg.\n");
#endif
        ttc::agent::UpdateConfigure *updateReq = (ttc::agent::UpdateConfigure *)msg;
        log_info("update configure to version %d, configure: \n%s",
                updateReq->version(), updateReq->configure().c_str());
        m_confManager->SetConfigure(updateReq->version(), updateReq->configure(),
                updateReq->configure_cksum());
        delete msg;
        
        ttc::agent::UpdateConfigureReply *reply = new ttc::agent::UpdateConfigureReply;
        reply->set_ok(1);
        me->QueueMessage(AC_UpdateConfigureReply, reply);
        return 0;
    }
    else if(type == AC_SetPush)
    {
        ttc::agent::SetPush *setReq = (ttc::agent::SetPush *)msg;
#ifdef DEBUG
        log_info("SetPush msg.");
        log_info("Set Push = %u", setReq->push());
#endif
        if(0 == setReq->push())
        {
            m_confManager->UnsetPush();
        }
        else
        {
            m_confManager->SetPush();
        }
        delete msg;
        
        ttc::agent::SetPushReply *reply = new ttc::agent::SetPushReply;
        reply->set_ok(1);
        me->QueueMessage(AC_SetPushReply, reply);
        return 0;
    }
	else if(type == AC_QueryAgentStatus)
	{
		ttc::agent::QueryAgentStatus *queryStatus = (ttc::agent::QueryAgentStatus *)msg;
		
		uint32_t id = queryStatus->id();
        delete msg;
        
        ttc::agent::QueryAgentStatusReply *reply = new ttc::agent::QueryAgentStatusReply;
		reply->set_id(id);
        m_statusUpdater->GetAgentStatus(reply);
        me->QueueMessage(AC_QueryAgentStatusReply, reply);
        return 0;
		
	}
    else
    {
#ifdef DEBUG
        log_info("msg type unkown.\n");
#endif
        delete msg;
        return -1;
    }
}

