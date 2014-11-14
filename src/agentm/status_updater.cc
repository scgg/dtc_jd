#include <cstring>
#include "mysql.h"
#include "errmsg.h"
#include "log.h"

#include "status_updater.h"


StatusUpdater::StatusUpdater(const char *host, int port, const char *path,
        const char *username, const char *password, const char *db)
{
    m_host = host ? strdup(host) : NULL;
    m_port = port;
    m_path = path ? strdup(path) : NULL;
    m_username = username ? strdup(username) : NULL;
    m_password = password ? strdup(password) : NULL;
    m_db = strdup(db);
    m_mysql = new MYSQL;
}

StatusUpdater::~StatusUpdater()
{
    free(m_host);
    free(m_path);
    free(m_username);
    free(m_password);
    free(m_db);
    delete m_mysql;
}

int StatusUpdater::Connect()
{
    mysql_init(m_mysql);
    unsigned int timeout = 1;   //1 second
    mysql_options(m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);

    if(!mysql_real_connect(m_mysql, m_host, m_username, m_password,
                m_db, m_port, m_path, 0))
    {
        log_error("connect to db error: %s", mysql_error(m_mysql));
        return -1;
    }
#ifdef DEBUG
    log_info("connected to db.\n");
#endif

    return 0;
}

void StatusUpdater::Update(const char *peer, ttc::agent::HeartBeat *heartbeat)
{
    char buf[1024];
    snprintf(buf, sizeof(buf) - 1, "insert into agent_status (id,ip,agt_ver,conf_ver,md5)values(%d, '%s', '%s', %d, '%s') "
			"on duplicate key update ip='%s',agt_ver='%s',conf_ver=%d,md5='%s',modify_time=now();",
			heartbeat->agent_id(),
            peer, heartbeat->version().c_str(), 
            heartbeat->configure_version(),
            heartbeat->configure_md5().c_str(),
			peer, heartbeat->version().c_str(), 
            heartbeat->configure_version(),
            heartbeat->configure_md5().c_str());

    int rtn = mysql_real_query(m_mysql, buf, strlen(buf));

    if(rtn != 0)
    {
        log_error("update db error: %s", mysql_error(m_mysql));

        if(rtn == CR_SERVER_GONE_ERROR || rtn == CR_SERVER_LOST ||
                rtn == CR_UNKNOWN_ERROR)
        {
                mysql_close(m_mysql);
               int ret=Connect();
               if(ret == 0)
               {
            	   rtn = mysql_real_query(m_mysql, buf, strlen(buf));
               }
               else
               {
            	   	  log_error("reconnect db fail");
            	   	  return;
               }
        }
    }
#ifdef DEBUG
    log_info("update to db.\n");
#endif
}

void StatusUpdater::GetAgentStatus(ttc::agent::QueryAgentStatusReply *status)
{
	char buf[1024];
    snprintf(buf, sizeof(buf) - 1, "select unix_timestamp(),unix_timestamp(modify_time) from agent_status where id=%d;",
			status->id());

    int rtn = mysql_real_query(m_mysql, buf, strlen(buf));

    if(rtn != 0)
    {
        log_error("update db error: %s", mysql_error(m_mysql));

        if(rtn == CR_SERVER_GONE_ERROR || rtn == CR_SERVER_LOST ||
                rtn == CR_UNKNOWN_ERROR)
        {
                mysql_close(m_mysql);
                int ret=Connect();
                if(ret == 0)
                {
                	rtn =mysql_real_query(m_mysql, buf, strlen(buf));
                }
                else
                {
                	//重新连接数据库失败，将time设置为0，返回
                	log_error("reconnect db fail");
                	status->set_dbtime(0);
                	status->set_lmtime(0);
                	return;
                }
        }
    }
	
	MYSQL_RES*	result = mysql_use_result(m_mysql);
	
	if (result == NULL)
	{
		status->set_dbtime(0);
		status->set_lmtime(0);
		return;
	}
	MYSQL_ROW	row;
	int count = 0;
	while ((row = mysql_fetch_row(result)))
	{
		if (row[0] == NULL)
		{
			continue;
		}
		
		status->set_dbtime(atoll(row[0]));
		status->set_lmtime(atoll(row[1]));
		count++;
	}
	
	if (count == 0)
	{
		status->set_dbtime(0);
		status->set_lmtime(0);
	}
	
	mysql_free_result(result);
}

