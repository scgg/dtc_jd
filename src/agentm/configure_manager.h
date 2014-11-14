// Author: yizhiliu
// Date: 

#ifndef CONFIGURE_MANAGER_H__
#define CONFIGURE_MANAGER_H__

#include <string>
#include <stdio.h>
#include "mysql.h"

class ConfigureManager
{
public:
    int LatestConfigureVersion() { return m_configureVersion; }
    const std::string &LatestConfigure() { return m_configure; }
    const std::string &LatestConfigureMd5() { return m_configureMd5; }

    void SetPush() { m_push = true; }
    void UnsetPush() { m_push = false; }
    bool PushSetted() { return m_push; }
    int  Init(const char *dbHost, int dbPort, const char *dbPath, const char *dbUser, const char *dbPassword, const char *dbName)
    {
    	if(dbHost)
    		m_dbHost = dbHost;
    	m_dbPort = dbPort;
    	if(dbUser)
    		m_dbUser = dbUser;
    	if(dbPassword)
    		m_dbPassword = dbPassword;
    	if(dbName)
    		m_dbName = dbName;
		if (dbPath)
			m_dbPath = dbPath;

    	return FetchConfigInfoFromDatabase();
    }

    void SetConfigure(int version, const std::string &configure,
            const std::string &cksum)
    {
        m_configureVersion = version;
        m_configure = configure;
        m_configureMd5 = cksum;
    }

    ConfigureManager() : m_configureVersion(-1), m_push(false) {}

    int FetchConfigInfoFromDatabase()
    {
        MYSQL mysql;
        if(!mysql_init(&mysql))
        {
            fprintf(stderr, "mysql init error: %s\n", mysql_error(&mysql));
            mysql_close(&mysql);
            return -1;
        }
        const char *dbPath = NULL;
        if (m_dbPath != "")
        	dbPath = m_dbPath.c_str();
        if(!mysql_real_connect(&mysql, m_dbHost.c_str(), m_dbUser.c_str(), m_dbPassword.c_str(), m_dbName.c_str(),
                    m_dbPort, dbPath, 0))
        {
            fprintf(stderr, "mysql connect error: %s\n", mysql_error(&mysql));
            mysql_close(&mysql);
            return -2;
        }

        const char *query = "select version, md5, file from agent_conf order by version desc limit 1";

        if(mysql_real_query(&mysql, query, strlen(query)))
        {
            fprintf(stderr, "select error: %s\n", mysql_error(&mysql));
            mysql_close(&mysql);
            return -3;
        }

        MYSQL_RES *res = mysql_store_result(&mysql);
        if(!res)
        {
            fprintf(stderr, "mysql_store_result error: %s\n", mysql_error(&mysql));
            mysql_close(&mysql);
            return -4;
        }

        if(mysql_num_rows(res) != 1)
        {
            fprintf(stderr, "mysql_num_rows got %d rows\n", (int)mysql_num_rows(res));
            mysql_close(&mysql);
            mysql_free_result(res);
            return -5;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        //unsigned long *lengths = mysql_fetch_lengths(res);
        int version = atoi(row[0]);
        std::string cksum = row[1];
        std::string configure = row[2];

        SetConfigure(version, configure, cksum);
        //printf("%d %s\n", version, configure.c_str());

        mysql_free_result(res);
        mysql_close(&mysql);

        return 0;
    }

private:
    int m_configureVersion;
    bool m_push;
    std::string m_configure;
    std::string m_configureMd5;
    //db info add by linjinming 2014-05-31
    std::string m_dbHost;
    std::string m_dbUser;
    std::string m_dbPassword;
    std::string m_dbName;
    std::string m_dbPath;
    int         m_dbPort;

};


#endif

