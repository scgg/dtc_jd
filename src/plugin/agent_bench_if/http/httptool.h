/*
 * httptool.h
 *
 *  Created on: 2014年8月15日
 *      Author: p_xiangxli
 */

struct RequestBody {
	std::string acsToken;
	std::string tableName;
	int keyType;
	std::string strKey;
	int intKey;
};

class HttpTool {
private:
	static int test;
	static std::map<std::string, NCServer*> serverGroup;
	static int setKeyType(NCServer* server, const int keyType);
	static int MakeJsonField(NCResult* result, const int ftype, const char* fname, Json::Value& packet);

public:
	static int Parse(const std::string body, RequestBody& reqBody);
	static NCServer* getNCServer(const RequestBody reqBody);
	static int doRequest(NCServer* server, const RequestBody reqBody, char** send_buf, int* send_len);
	static void doError(char **send_buf, int *send_len);
};



