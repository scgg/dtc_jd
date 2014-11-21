#ifndef MYSQL_MANAGER_H
#define MYSQL_MANAGER_H

#include <string>
using namespace std;

class MySqlManager{
public:
	MySqlManager(string host, string user, string password, int port, string name);
	~MySqlManager();
	int GetConnection(string& errMsg);
	void DestroyConnection();
	int ExecuteSql(const string& query, string& result, string& errMsg);
private:
	void *connection;
	//
	string dbHost;
	string dbUser;
	string dbPassword;
	string dbName;
	unsigned int dbPort;
	//
	bool bConnected;
};

#endif
