// Author: yizhiliu
// Date: 

#ifndef STATUS_UPDATER_H__
#define STATUS_UPDATER_H__

#include <string>

#include "admin_protocol.pb.h"

typedef struct st_mysql MYSQL;

class StatusUpdater
{
public:
    StatusUpdater(const char *host, int port, 
            const char *path,
            const char *username, const char *password, const char *db);
    ~StatusUpdater();

    int Connect();

    void Update(const char *peer, ttc::agent::HeartBeat *heartbeat);
	void GetAgentStatus(ttc::agent::QueryAgentStatusReply *status);
private:
    char *m_host;
    int m_port;
    char *m_path;
    char *m_username;
    char *m_password;
    char *m_db;

    MYSQL *m_mysql;
};


#endif

