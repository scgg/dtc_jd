#include "write_data.h"
#include "pod.h"
#include <sstream>
#include "LockFreeQueue.h"
#include "log.h"
#include "mysql_manager.h"
#include "helper.h"
#include "dtchttpd_alarm.h"

extern MySqlManager* g_manager;
extern LockFreeQueue<MSG> g_Queue;
//extern LockFreeQueue<InsertDo> g_insertQueue;

WriteData::WriteData(const char* name) : CThread(name, CThread::ThreadTypeAsync)
{
}

WriteData::~WriteData()
{
}

void* WriteData::Process()
{
	while(false == Stopping())
	{
		log_info("enter writedata process");
		MSG data;
		bool ret = g_Queue.DeQueue(data);
		if (true == ret)
		{
			//get sql query
			std::ostringstream query_stream;
			switch(data.cmd)
			{
				case 0:
					{
						std::string currenttime = GetCurrentFormatTime();
						std::string tablename = "t_monitor_" + currenttime.substr(0,4) + currenttime.substr(5,2);
						query_stream << "insert into " << tablename << "(bid,curve,etype,data1,data2,extra,datetime)"  << " values (" << data.bid << "," 
									 << data.curve << "," << data.etype << "," << data.data1 << "," << data.data2 << "," 
									 << data.extra << "," << data.datetime << ");";
						break;
					}
				case 1:
					{
						query_stream << "replace into t_monitor_instant set bid=" << data.bid << ",type=" << data.curve 
									 << ",data1=" << data.data1 << ",data2=" << data.data2 << ",extra=" << data.extra
									 << ",datetime=" << data.datetime << ";";
						break;
					}
				case 2:
					{
						query_stream << "replace into t_monitor_check set type=" << data.curve << ",datetime=" << data.datetime << ";";
						break;
					}
				default:
					{
						log_error("invalid cmd: %d", data.cmd);
					}
			}
			//execute sql query
			std::string query = query_stream.str();
			if (query.empty())
			{
				log_error("query is empty, query is %s", query.c_str());
				continue;
			}
			if (g_manager->ExecuteSql(query) < 0)
			{
				log_error("execute mysql failed. query is %s", query.c_str());
				g_Queue.EnQueue(data);
			}
			//lock free queue size alarm
			if (g_Queue.Size()*2 > g_Queue.QueueSize())
			{
				log_error("queue size is %u has use %u", g_Queue.QueueSize(), g_Queue.Size());
				dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::QUEUE_CONGEST, g_Queue.QueueSize(), g_Queue.Size());
			}
			//thread cpu rate alarm
			if (true == dtchttpd::Alarm::GetInstance().ScanCpuThreadStat())
			{
				log_error("dtchttpd cpu use rate too high");
				dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::CPU_RATE_HIGH);
			}
			log_info("exit writedata process");
		}
		else
		{
			log_info("get queue element fail, queue size is %u has use %u", g_Queue.QueueSize(), g_Queue.Size());
			sleep(2);
			continue;
		}
	}
	return 0;
}
