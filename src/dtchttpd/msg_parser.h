#ifndef MSG_PARSER_H
#define MSG_PARSER_H

#include <string>
#include "json/json.h"

class MsgParser
{
public:	
	static int Parse(const std::string& content);
	static int ProcessSingle(int bid, const Json::Value &value);
	static int ProcessBatch(const Json::Value &value);
	
private:
	MsgParser();
	MsgParser(const MsgParser&);
	MsgParser& operator=(const MsgParser&);
	
};

#endif
