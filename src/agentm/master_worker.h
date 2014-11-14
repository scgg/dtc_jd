// Author: yizhiliu
// Date: 

#ifndef MASTER_WORKER_H__
#define MASTER_WORKER_H__

#include <string>

#include "protocol_handler.h"

class StatusUpdater;
class ConfigureManager;

class MasterWorker : public IProtocolProcessor
{
public:
    MasterWorker(const char *peer, StatusUpdater *statusUpdater,
            ConfigureManager *confManager) : m_remoteAddr(peer),
    m_statusUpdater(statusUpdater), m_confManager(confManager) {}
    int HandleMessage(ProtocolHandler *me,
            uint32_t type, google::protobuf::Message *msg);

private:
    std::string m_remoteAddr;
    StatusUpdater *m_statusUpdater;
    ConfigureManager *m_confManager;
};

#endif

