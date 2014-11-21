#include "msg_parser.h"
#include "log.h"
#include <stdlib.h>
#include "LockFreeQueue.h"
#include "pod.h"
#include "helper.h"

extern LockFreeQueue<MSG> g_Queue;
extern std::string g_another_dtchttpd_url;
extern __gnu_cxx::hash_set<int> g_gray_bids;

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
	//concatenation to support gray mode
	int curve_value = atoi(value["curve"].asString().c_str());
	if ((false == g_gray_bids.empty()) && (g_gray_bids.find(bid) != g_gray_bids.end()))
	{
		log_info("in gray mode, bid: %d", bid);
		Json::FastWriter writer;
		std::string post_content = writer.write(value);
		return DoHttpRequest(g_another_dtchttpd_url, post_content);
	}
	//normal process
	MSG msg;
	msg.bid = bid;
	if (1 == atoi(value["cmd"].asString().c_str())) //update
	{
		msg.curve = curve_value;
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
	//check dtchttpd healthy
	if (1000 == curve_value)
	{
		msg.curve = curve_value;
		msg.datetime = atol(value["datetime"].asString().c_str());
		msg.cmd = 2;
		log_info("bid: %d curve: %d datetime: %ld", msg.bid, curve_value, msg.datetime);
		bool ret = g_Queue.EnQueue(msg);
		if (false == ret)
		{
			log_error("curve: %d datetime: %ld enqueue failed", msg.curve, msg.datetime);
			return -1;
		}
		return 0;
	}
	//ping dtchttpd
	if (1001 == curve_value)
	{
		log_info("curve: %d", curve_value);
		return 0;
	}
	//insert curves
	msg.curve = curve_value;
	if ((msg.curve<1)||(msg.curve>8)) //must have, if out of range may cause program crash
	{
		log_error("unknown insert curve type, curve is %d", msg.curve);
		return -1;
	}
	msg.etype = atoi(value["etype"].asString().c_str());
	msg.data1 = atoll(value["data1"].asString().c_str());
	msg.data2 = atoll(value["data2"].asString().c_str());
	msg.extra = atoll(value["extra"].asString().c_str());
	long now_time = GetUnixTime();
	//insert data
	if ((msg.curve==1) || (msg.curve==2) || (msg.curve==5))
	{
		if (msg.data2 == 0)
		{
			log_info("not need 10s data. bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
					 msg.bid, msg.curve, msg.etype, msg.data1, msg.data2, msg.extra, msg.datetime);
			goto ALARM_SECTION;
		}
	}
	if ((msg.data1==0) && (msg.data2==0) && (msg.extra==0))
	{
		log_info("not need 10s data. bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
				 msg.bid, msg.curve, msg.etype, msg.data1, msg.data2, msg.extra, msg.datetime);
		goto ALARM_SECTION;
	}
	if (value["datetime"].asString().empty())
		msg.datetime = now_time/10*10;
	else
		msg.datetime = atol(value["datetime"].asString().c_str())/10*10;
	msg.cmd = 0;
	if (false == g_Queue.EnQueue(msg))
	{
		log_error("g_Queue enqueue 10s data failed");
	}
	log_info("insert 10s data success. bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
			 msg.bid, msg.curve, msg.etype, msg.data1, msg.data2, msg.extra, msg.datetime);
	ALARM_SECTION:
	//alarm data
	if (value["datetime"].asString().empty())
		msg.datetime = now_time/300*300;
	else
		msg.datetime = atol(value["datetime"].asString().c_str())/300*300;
	msg.cmd = 3;
	msg.extra = 1;
	if (false == g_Queue.EnQueue(msg))
	{
		log_error("g_Queue enqueue 5min data failed");
	}
	log_info("insert 5min data success. bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
			 msg.bid, msg.curve, msg.etype, msg.data1, msg.data2, msg.extra, msg.datetime);
	return 0;
}
