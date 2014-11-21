#ifndef MYSQL_MANAGER_H
#define MYSQL_MANAGER_H

#include <string>

class MySqlManager
{
public:
	MySqlManager(std::string host, std::string user, std::string password, int port, std::string name);
	MySqlManager(std::string host, std::string user, std::string password, int port);
	~MySqlManager();
	
public:
	int GetConnection();
	int Open(std::string db_name="");
	void DestroyConnection();
	
public:
	int ExecuteSql(std::string query);

public:
	int UseResult();
	int FetchRow(char ***row);
	int FreeResult();
	int GetRowNum();
	unsigned int GetFieldCount();
	
public:
	int BeginTransaction();
	int Commit();
	int RollBack();
	
private:
	void *connection;
	bool bConnected;
	//
	std::string dbHost;
	std::string dbUser;
	std::string dbPassword;
	std::string dbName;
	unsigned int dbPort;
	//
	void *res;
	int row_num;
	bool bFreeResult;
};

#endif
