#include "mysql_manager.h"
#include "log.h"
#include <string.h>
#include "mysql.h"
#include <stdlib.h>


MySqlManager::MySqlManager(std::string host, std::string user, std::string password, int port, std::string name) : 
						   dbHost(host), dbUser(user), dbPassword(password), dbName(name), dbPort(port), bConnected(false)
{
	connection = malloc(sizeof(MYSQL));
}

MySqlManager::~MySqlManager()
{
	this->DestroyConnection();
	free(connection);
}

int MySqlManager::GetConnection()
{
	if (true == bConnected)
	{
		return 0;
	}
	log_info("init mysql connection");
	if(!mysql_init((MYSQL *)connection))
	{
		log_error("mysql init failed.");
		return -1;
	}
	if(!mysql_real_connect((MYSQL *)connection, dbHost.c_str(), dbUser.c_str(), dbPassword.c_str(), dbName.c_str(), dbPort, NULL, 0))
	{
		log_error("mysql connect failed.");
		mysql_close((MYSQL *)connection);
		return -1;
	}
	bConnected = true;
	return 0;
}

void MySqlManager::DestroyConnection()
{
	mysql_close((MYSQL *)connection);
	bConnected = false;
}

int MySqlManager::ExecuteSql(std::string query)
{
	if (false == bConnected)
	{
		log_error("mysql not connected");
		if (GetConnection() < 0)
		{
			log_error("init mysql connetion failed");
			return -1;
		}
	}
	if (true == query.empty())
	{
		log_error("empty query");
		return -1;
	}
	if(mysql_real_query((MYSQL *)connection, query.c_str(), strlen(query.c_str())))
	{
		log_error("mysql query failed.");
		DestroyConnection();
		return -1;
	}
	return 0;
}
