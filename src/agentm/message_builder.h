// Author: yizhiliu
// Date: 

#ifndef AGENTM_MESSAGE_BUILDER_H__
#define AGENTM_MESSAGE_BUILDER_H__

#include <google/protobuf/message.h>
#include "admin.h"
#include "admin_protocol.pb.h"

inline google::protobuf::Message *CreateMessage(uint32_t type)
{
    switch(type)
    {
    case AC_HeartBeat:
        return new ttc::agent::HeartBeat();
    case AC_HeartBeatReply:
        return new ttc::agent::HeartBeatReply();
    case AC_GetLatestConfigure:
        return new ttc::agent::GetLatestConfigure();
    case AC_LatestConfigureReply:
        return new ttc::agent::LatestConfigureReply();
    case AC_UpdateConfigure:
        return new ttc::agent::UpdateConfigure();
    case AC_UpdateConfigureReply:
        return new ttc::agent::UpdateConfigureReply();

    case AC_SetPush:
        return new ttc::agent::SetPush();
    case AC_SetPushReply:
        return new ttc::agent::SetPushReply();
	case AC_QueryAgentStatus:
		return new ttc::agent::QueryAgentStatus();
	case AC_QueryAgentStatusReply:
		return new ttc::agent::QueryAgentStatusReply();

    default:
        //should not got here
        return NULL;
    }
}

#endif

