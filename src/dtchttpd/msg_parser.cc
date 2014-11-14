#include "msg_parser.h"
#include "log.h"
#include <stdlib.h>
#include "curve.h"
#include "LockFreeQueue.h"
#include "pod.h"
#include "helper.h"

extern std::vector<Curve> g_curves;
extern LockFreeQueue<MSG> g_Queue;

int MsgParser::Parse(const std::string& content)
{
	//parse http post body, it's json format
	Json::Value value;
	Json::Reader reader;
	int ret = reader.parse(content, value, false);
	if (0 == ret)
	{
		log_error("msg parse failed. msg: %s", content.c_str());
		return -1;
	}
	//cmd {0,1,2} 0 insert 1 update 2 command 3 batch command
	if (3 == atoi(value["cmd"].asString().c_str())) //batch command
	{
		return ProcessBatch(value);
	}
	else //single command
	{
		return ProcessSingle(atoi(value["bid"].asString().c_str()), value);
	}
}

int MsgParser::ProcessBatch(const Json::Value &value)
{
	if (value["statics"].isArray())
	{
		Json::Value bid_value;
		int bid;
		for (int i=0; i<value["statics"].size(); i++)
		{
			bid_value = value["statics"][i];
			bid = atoi(bid_value["bid"].asString().c_str());
			Json::Value content_value;
			for(int j=0; j<bid_value["content"].size(); j++)
			{
				content_value = bid_value["content"][j];
				ProcessSingle(bid, content_value);
			}
		}
	}
	else
	{
		log_error("error json format: %s", value.toStyledString().c_str());
		return -1;
	}
	return 0;
}

int MsgParser::ProcessSingle(int bid, const Json::Value &value)
{
	MSG msg;
	msg.bid = bid;
	if (1 == atoi(value["cmd"].asString().c_str())) //update
	{
		msg.curve = atoi(value["curve"].asString().c_str());
		msg.data1 = atoll(value["data1"].asString().c_str());
		msg.data2 = atoll(value["data2"].asString().c_str());
		msg.extra = atoll(value["extra"].asString().c_str());
		if (value["datetime"].asString().empty())
			msg.datetime = GetUnixTime()/10*10;
		else
			msg.datetime = atol(value["datetime"].asString().c_str())/10*10;
		msg.cmd = 1;
		log_info("bid: %d type: %d data1: %lld data2: %lld extra: %lld datetime: %ld cmd: %d", 
				 msg.bid, msg.curve, msg.data1, msg.data2, msg.extra, msg.datetime, msg.cmd);
		bool ret = g_Queue.EnQueue(msg);
		if (false == ret)
		{
			log_error("bid: %d type: %d data1: %lld data2: %lld extra: %lld datetime: %ld cmd: %d", 
					 msg.bid, msg.curve, msg.data1, msg.data2, msg.extra, msg.datetime, msg.cmd);
			return -1;
		}
		return 0;
	}
	int type = atoi(value["curve"].asString().c_str());
	//check dtchttpd alive
	if (1000 == type)
	{
		msg.curve = atoi(value["curve"].asString().c_str());
		msg.datetime = atol(value["datetime"].asString().c_str());
		msg.cmd = 2;
		log_info("bid: %d curve: %d datetime: %ld", msg.bid, type, msg.datetime);
		bool ret = g_Queue.EnQueue(msg);
		if (false == ret)
		{
			log_error("curve: %d datetime: %ld enqueue failed", msg.curve, msg.datetime);
			return -1;
		}
		return 0;
	}
	//insert curves
	msg.curve = atoi(value["curve"].asString().c_str());
	if ((msg.curve<1)||(msg.curve>8))
	{
		log_error("unknown insert curve type, curve is %d", msg.curve);
		return -1;
	}
	msg.etype = atoi(value["etype"].asString().c_str());
	msg.data1 = atoll(value["data1"].asString().c_str());
	msg.data2 = atoll(value["data2"].asString().c_str());
	msg.extra = atoll(value["extra"].asString().c_str());
	if (value["datetime"].asString().empty())
		msg.datetime = GetUnixTime()/10*10;
	else
		msg.datetime = atol(value["datetime"].asString().c_str())/10*10;
	msg.cmd = 0;
	log_info("bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
			 msg.bid, msg.curve, msg.etype, msg.data1, msg.data2, msg.extra, msg.datetime);
	return g_curves[msg.curve-1].ProcessData(msg);
}
