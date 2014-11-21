#include "mysql_manager.h"
#include "log.h"
#include <string.h>
#include "mysql.h"
#include <stdlib.h>
#include <sstream>
#include "http_agent_error_code.h"
#include "http_common.h"

using namespace std;

MySqlManager::MySqlManager(string host, string user, string password, int port, string name) :
						   dbHost(host), dbUser(user), dbPassword(password), dbName(name), dbPort(port), bConnected(false){
	connection = malloc(sizeof(MYSQL));
}

MySqlManager::~MySqlManager(){
	this->DestroyConnection();
	free(connection);
}

int MySqlManager::GetConnection(string& errMsg){
	int ret = 0;
	if (true == bConnected){
		return 0;
	}
	log_info("init mysql connection");
	if(!mysql_init((MYSQL *)connection)){
		ret = ERROR_CODEFACTORY_INIT_MYSQL_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("mysql init failed. errCode: %d, errMsg: %s", ret, errMsg.c_str());
		return ret;
	}
	if(!mysql_real_connect((MYSQL *)connection, dbHost.c_str(), dbUser.c_str(), dbPassword.c_str(), dbName.c_str(), dbPort, NULL, 0)){
		ret = ERROR_CODEFACTORY_MYSQL_CONNECT_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("mysql connect failed. errCode: %d, errMsg: %s", ret, errMsg.c_str());
		mysql_close((MYSQL *)connection);
		return ret;
	}
	bConnected = true;
	return ret;
}

void MySqlManager::DestroyConnection(){
	mysql_close((MYSQL *)connection);
	bConnected = false;
}

int MySqlManager::ExecuteSql(const string& query, string& sqlRes, string& errMsg){
	int ret = 0;
	if (false == bConnected){
		log_error("mysql not connected");
		if (GetConnection(errMsg) < 0){
			ret = ERROR_CODEFACTORY_INIT_MYSQL_CONNECTION_FAIL;
			errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
			log_error("init mysql connetion failed. errCode: %d, errMsg: %s", ret, errMsg.c_str());
			return ret;
		}
	}
	if (true == query.empty()){
		ret = ERROR_CODEFACTORY_EMPTY_QUERY_SQL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("empty query. errCode: %d, errMsg: %s", ret, errMsg.c_str());
		return ret;
	}
	if(mysql_real_query((MYSQL *)connection, query.c_str(), strlen(query.c_str()))){
		ret = ERROR_CODEFACTORY_EXECUTE_SQL_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("mysql query failed. errCode: %d, errMsg: %s", ret, errMsg.c_str());
		DestroyConnection();
		return ret;
	}
	MYSQL_RES *result = mysql_store_result((MYSQL *)connection);
	if(result == NULL){
		ret = ERROR_CODEFACTORY_EXECUTE_SQL_RESULT_EMPTY;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("mysql empty result. errCode: %d, errMsg: %s", ret, errMsg.c_str());
		DestroyConnection();
		return ret;
	}
	MYSQL_ROW row;
	row = mysql_fetch_row(result);
	if(row != NULL){
		log_info("row[0]: %s", row[0]);
		char* rst = row[0];
		sqlRes = rst;
	}
	log_info("sqlRes:%s", sqlRes.c_str());
	mysql_free_result(result);
	return ret;
}
