#ifndef WORKER_H_
#define WORKER_H_

#include <string>
#include "json/json.h"
#include <fstream>

using namespace std;

class Worker;
class MsgParser
{
public:
	MsgParser();
	~MsgParser();
	int Parse(const string& content, int& bid, int& cmd, string& ip, string& port, string& token, string& errMsg);

};

class Worker{
public:
	Worker();
	~Worker();
	int GetTableConf(const int& bid, string& tableconf, string& errMsg);
	int MakeTplCode(
			const string& tableconf,
			const int& cmd,
			const int& bid,
			const string& ip,
			const string& port,
			const string& token,
			string& tplCodeDir,
			string& errMsg);
private:

};

#endif
