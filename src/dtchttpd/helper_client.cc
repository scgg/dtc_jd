#include "helper_client.h"
#include <string>
#include <sstream>
#include "log.h"
#include <stdlib.h>
#include "config.h"

using namespace dtchttpd;

extern dtchttpd::CConfig g_storage_config;

CHelperClient::CHelperClient(const char *name, HelperConfig config, LockFreeQueue<MSG> *queue) : CThread(name, CThread::ThreadTypeAsync)
{
	m_mysql_manager = new MySqlManager(config.ip, config.user, config.pass, config.port);
	m_mysql_manager->Open();
	m_msg_queue = queue;
}

CHelperClient::~CHelperClient()
{
	if (NULL != m_mysql_manager)
	{
		delete m_mysql_manager;
		m_mysql_manager = NULL;
	}
}

void* CHelperClient::Process()
{
	int db_index;
	std::string db_name;
	while(false == Stopping())
	{
		MSG data;
		bool ret = m_msg_queue->DeQueue(data);
		if (true == ret)
		{
			//get db name
			if ((3==data.cmd) || (2==data.cmd))
				db_index = 0;
			else
				db_index = data.bid%g_storage_config.GetDBMod();
			std::ostringstream db_ss;
			db_ss << "dtc_monitor_" << db_index;
			db_name = db_ss.str();
			//select db
			if (m_mysql_manager->Open(db_name) < 0)
			{
				log_error("select db failed");
				continue;
			}
			//get sql query
			switch(data.cmd)
			{
				case 0: InsertTask(data);  break;
				case 1: ReplaceTask(data); break;
				case 2: CheckTask(data);   break;
				case 3: AlarmTask(data);   break;
				default: log_error("invalid cmd: %d", data.cmd);
			}
		}
		else
		{
			log_info("get queue element fail, queue size is %u has use %u", m_msg_queue->QueueSize(), m_msg_queue->Size());
			sleep(2);
			continue;
		}
	}
	return 0;
}

int CHelperClient::InsertTask(const MSG &data)
{
	log_info("begin insert task");
	//get table name
	std::ostringstream table_stream;
	table_stream << "t_monitor_" << (data.bid%g_storage_config.GetTabMod());
	std::string table_name = table_stream.str();
	//begin
	if (m_mysql_manager->BeginTransaction() < 0)
	{
		log_error("begin transaction failed");
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql begin transaction success");
	//gap lock
	std::ostringstream select_for_update_stream;
	select_for_update_stream << "select data1,data2,extra from " << table_name << " where datetime=" << data.datetime << 
													  " and bid="        << data.bid      << 
													  " and curve="      << data.curve    <<
													  " for update;";
	std::string query = select_for_update_stream.str();
	if (m_mysql_manager->ExecuteSql(query) < 0)
	{
		log_error("execut mysql query failed. query is %s", query.c_str());
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql select for update success, query is %s", query.c_str());
	//get result
	if (m_mysql_manager->UseResult() < 0)
	{
		log_error("can not get result");
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql use result success");
	//insert or update
	std::ostringstream insert_stream;
	if (0 == m_mysql_manager->GetRowNum()) //first insert
	{
		log_info("row num is 0, first insert");
		insert_stream << "insert into " << table_name << "(bid,curve,etype,data1,data2,extra,datetime)"  << " values (" << data.bid << "," 
						 << data.curve << "," << data.etype << "," << data.data1 << "," << data.data2 << "," << data.extra << "," << data.datetime << ");";
		query = insert_stream.str();
		if (m_mysql_manager->ExecuteSql(query) < 0)
		{
			log_error("execute mysql query failed. query is %s", query.c_str());
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql first insert success");
	}
	else //update
	{
		log_info("row num is %d, update row", m_mysql_manager->GetRowNum());
		char **row;
		if (m_mysql_manager->FetchRow(&row) < 0)
		{
			log_error("mysql get row failed");
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql fetch row success");
		log_info("current row field count is %u", m_mysql_manager->GetFieldCount());
		log_info("row[0] %lld row[1] %lld row[2] %lld", atoll(row[0]), atoll(row[1]), atoll(row[2]));
		insert_stream << "update " << table_name << " set data1="  << (data.data1 + atoll(row[0]))
												 << ",data2="      << (data.data2 + atoll(row[1]))
												 << ",extra="      << (data.extra + atoll(row[2])) << " where datetime=" 
												 << data.datetime << " and bid=" << data.bid << " and curve=" << data.curve << ";";
		query = insert_stream.str();
		if (m_mysql_manager->ExecuteSql(query) < 0)
		{
			log_error("execute mysql query failed. query is %s", query.c_str());
			m_mysql_manager->FreeResult();
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql update success");
	}
	//free result
	m_mysql_manager->FreeResult();
	//commit
	if (m_mysql_manager->Commit() < 0)
	{
		log_error("mysql commit failed.query is %s", query.c_str());
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("end insert task");
	return 0;
}

int CHelperClient::ReplaceTask(const MSG &data)
{
	log_info("start replace task");
	std::ostringstream query_stream;
	query_stream << "replace into t_monitor_instant set bid=" << data.bid << ",type=" << data.curve 
				 << ",data1=" << data.data1 << ",data2=" << data.data2 << ",extra=" << data.extra
				 << ",datetime=" << data.datetime << ";";
	std::string query = query_stream.str();
	if (m_mysql_manager->ExecuteSql(query) < 0)
	{
		log_error("execute mysql failed. query is %s", query.c_str());
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("end replace task");
	return 0;
}

int CHelperClient::CheckTask(const MSG &data)
{
	log_info("start check task");
	std::ostringstream query_stream;
	query_stream << "replace into t_monitor_check set type=" << data.curve << ",datetime=" << data.datetime << ";";
	std::string query = query_stream.str();
	if (m_mysql_manager->ExecuteSql(query) < 0)
	{
		log_error("execute mysql failed. query is %s", query.c_str());
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("end check task");
	return 0;
}

int CHelperClient::AlarmTask(const MSG &data)
{
	log_info("start alarm task");
	//begin
	if (m_mysql_manager->BeginTransaction() < 0)
	{
		log_error("begin transaction failed");
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql begin transaction success");
	//gap lock
	std::ostringstream select_for_update_stream;
	select_for_update_stream << "select data1,data2,extra from t_monitor_alarm where datetime=" << data.datetime << 
													  " and bid="        << data.bid      << 
													  " and curve="      << data.curve    <<
													  " for update;";
	std::string query = select_for_update_stream.str();
	if (m_mysql_manager->ExecuteSql(query) < 0)
	{
		log_error("execut mysql query failed. query is %s", query.c_str());
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql select for update success, query is %s", query.c_str());
	//get result
	if (m_mysql_manager->UseResult() < 0)
	{
		log_error("can not get result");
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("mysql use result success");
	//insert or update
	std::ostringstream insert_stream;
	if (0 == m_mysql_manager->GetRowNum()) //first insert
	{
		log_info("row num is 0, first insert");
		insert_stream << "insert into t_monitor_alarm(bid,curve,data1,data2,extra,datetime)"  << " values (" << data.bid << "," 
						 << data.curve << "," << data.data1 << "," << data.data2 << "," << data.extra << "," << data.datetime << ");";
		query = insert_stream.str();
		if (m_mysql_manager->ExecuteSql(query) < 0)
		{
			log_error("execute mysql query failed. query is %s", query.c_str());
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql first insert success");
	}
	else //update
	{
		log_info("row num is %d, update row", m_mysql_manager->GetRowNum());
		char **row;
		if (m_mysql_manager->FetchRow(&row) < 0)
		{
			log_error("mysql get row failed");
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql fetch row success, current row field count is %u row[0] %lld row[1] %lld row[2] %lld", 
			      m_mysql_manager->GetFieldCount(), atoll(row[0]), atoll(row[1]), atoll(row[2]));
		insert_stream << "update t_monitor_alarm set data1="  << (data.data1 + atoll(row[0]))
					  << ",data2=" << (data.data2 + atoll(row[1])) << ",extra=" << (data.extra + atoll(row[2])) 
					  << " where datetime=" << data.datetime << " and bid=" << data.bid << " and curve=" << data.curve << ";";
		query = insert_stream.str();
		if (m_mysql_manager->ExecuteSql(query) < 0)
		{
			log_error("execute mysql query failed. query is %s", query.c_str());
			m_mysql_manager->FreeResult();
			m_mysql_manager->RollBack();
			m_msg_queue->EnQueue(data);
			return -1;
		}
		log_info("mysql update success");
	}
	//free result
	m_mysql_manager->FreeResult();
	//commit
	if (m_mysql_manager->Commit() < 0)
	{
		log_error("mysql commit failed.query is %s", query.c_str());
		m_mysql_manager->RollBack();
		m_msg_queue->EnQueue(data);
		return -1;
	}
	log_info("end alarm task");
	return 0;
}
