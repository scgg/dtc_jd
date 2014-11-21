#include "mysql_manager.h"
#include "log.h"
#include <string.h>
#include "mysql.h"
#include <stdlib.h>


MySqlManager::MySqlManager(std::string host, std::string user, std::string password, int port, std::string name) : 
						   bConnected(false), dbHost(host), dbUser(user), dbPassword(password), dbName(name), dbPort(port), res(NULL), row_num(0), bFreeResult(false)
{
	connection = malloc(sizeof(MYSQL));
}

MySqlManager::MySqlManager(std::string host, std::string user, std::string password, int port) : 
						   bConnected(false), dbHost(host), dbUser(user), dbPassword(password), dbPort(port), res(NULL), row_num(0), bFreeResult(false)
{
	connection = malloc(sizeof(MYSQL));
}

MySqlManager::~MySqlManager()
{
	DestroyConnection();
	free(connection);
}

int MySqlManager::GetConnection()
{
	if (true == bConnected)
	{
		return 0;
	}
	log_info("init mysql connection");
	if(mysql_init((MYSQL *)connection) == NULL)
	{
		log_error("mysql init failed, error is %s", mysql_error((MYSQL *)connection));
		return -1;
	}
	if(mysql_real_connect((MYSQL *)connection, dbHost.c_str(), dbUser.c_str(), dbPassword.c_str(), dbName.c_str(), dbPort, NULL, 0) == NULL)
	{
		log_error("mysql connect failed, error is %s", mysql_error((MYSQL *)connection));
		return -1;
	}
	bConnected = true;
	return 0;
}

int MySqlManager::Open(std::string db_name)
{
	if (false == bConnected)
	{
		log_info("init mysql connection");
		if(mysql_init((MYSQL *)connection) == NULL)
		{
			log_error("mysql init failed, error is %s", mysql_error((MYSQL *)connection));
			return -1;
		}
		if(mysql_real_connect((MYSQL *)connection, dbHost.c_str(), dbUser.c_str(), dbPassword.c_str(), NULL, dbPort, NULL, 0) == NULL)
		{
			log_error("mysql connect failed, error is %s", mysql_error((MYSQL *)connection));
			return -1;
		}
		bConnected = true;
	}
	if (false == db_name.empty())
	{
		dbName = db_name;
		log_info("db name[%s] is not empty, select db", dbName.c_str());
		if(mysql_select_db((MYSQL *)connection, dbName.c_str()) != 0)
		{
			log_error("mysql selet db failed, error is %s", mysql_error((MYSQL *)connection));
			DestroyConnection();
			return -1;
		}
	}
	return 0;
}

void MySqlManager::DestroyConnection()
{
	if (true == bConnected)
	{
		mysql_close((MYSQL *)connection);
		bConnected = false;
	}
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
	if(mysql_real_query((MYSQL *)connection, query.c_str(), strlen(query.c_str())) != 0)
	{
		log_error("mysql query failed, error is %s", mysql_error((MYSQL *)connection));
		DestroyConnection();
		return -1;
	}
	return 0;
}

int MySqlManager::BeginTransaction()
{
	return ExecuteSql("START TRANSACTION;");
}

int MySqlManager::Commit()
{
	return ExecuteSql("COMMIT;");
}

int MySqlManager::RollBack()
{
	return ExecuteSql("ROLLBACK;");
}

int MySqlManager::UseResult()
{
	res = (void*)mysql_store_result((MYSQL *)connection);
	if(NULL == res)
	{
		if(mysql_errno((MYSQL *)connection) != 0)
		{
			log_error("mysql_store_result failed, error is %s", mysql_error((MYSQL *)connection));
			DestroyConnection();
			return -1;
		}
		else
		{
			row_num = 0;
			return 0;
		}
	}
	
	row_num = mysql_num_rows((MYSQL_RES *)res);
	if(row_num < 0)
	{
		log_error("mysql_num_rows less than 0, error is %s", mysql_error((MYSQL *)connection));
		mysql_free_result((MYSQL_RES *)res);
		DestroyConnection();
		return -1;
	}
	bFreeResult = true;

	return 0;
}

int MySqlManager::GetRowNum()
{
	return row_num;
}

int MySqlManager::FetchRow(char ***row)
{
	*row = (char **)mysql_fetch_row((MYSQL_RES *)res);
	if(NULL == row)
	{
		log_error("mysql_fetch_row failed, error is %s", mysql_error((MYSQL *)connection));
		FreeResult();
		DestroyConnection();
		return -1;			
	}
	
	return 0;
}

int MySqlManager::FreeResult()
{
	if(bFreeResult)
	{
		mysql_free_result((MYSQL_RES *)res);
		bFreeResult = false;
	}
	return 0;
}


unsigned int MySqlManager::GetFieldCount()
{
	return mysql_num_fields((MYSQL_RES *)res);
}

