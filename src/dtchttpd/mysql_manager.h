#ifndef MYSQL_MANAGER_H
#define MYSQL_MANAGER_H

#include <string>

class MySqlManager
{
public:
	MySqlManager(std::string host, std::string user, std::string password, int port, std::string name);
	~MySqlManager();
	int GetConnection();
	void DestroyConnection();
	int ExecuteSql(std::string query);
private:
	void *connection;
	//
	std::string dbHost;
	std::string dbUser;
	std::string dbPassword;
	std::string dbName;
	unsigned int dbPort;
	//
	bool bConnected;
};

#endif
