#ifndef AGENT_ADMIN_H__
#define AGENT_ADMIN_H__

#include <stdint.h>

enum AdminCode {
    AC_Begin,
    AC_Reply,
    AC_AddModule,
    AC_RemoveModule,
    AC_AddCacheServer,
    AC_RemoveCacheServer,
    AC_ReplaceCacheServer,
    AC_ReloadConfig,

    AC_HeartBeat,
    AC_HeartBeatReply,
    AC_GetLatestConfigure,
    AC_LatestConfigureReply,
	AC_UpdateConfigure,
    AC_UpdateConfigureReply,
    AC_SetPush,
    AC_SetPushReply,
	AC_QueryAgentStatus,
	AC_QueryAgentStatusReply,
	AC_Ping,
	AC_DumpConfig,
	AC_SwitchMechine,
    AC_CmdEnd,
};

enum AdminResult {
    AdminOk = 0,
    HostExist,
    AdminFail,
    BadAdminCmd,
    ModuleNotFound,
    ModuleAlreadyExist,
    CmdUnkown,
    InvalidConfigFile,
};

struct ProtocolHeader
{
    uint32_t magic;
    uint32_t length;
    uint32_t cmd;
};

const uint32_t ADMIN_PROTOCOL_MAGIC = 0x54544341;   //TTCA

inline
bool IsProtocolHeaderValid(const ProtocolHeader *header)
{
    return header->magic == ADMIN_PROTOCOL_MAGIC &&
        header->cmd > AC_Begin && header->cmd < AC_CmdEnd;
}

#endif
