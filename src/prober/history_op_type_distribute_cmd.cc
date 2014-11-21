#include "history_op_type_distribute_cmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(HistoryOpTypeDistributeCmd)

HistoryOpTypeDistributeCmd::HistoryOpTypeDistributeCmd(void)
{
	m_ver = 40304;
}


HistoryOpTypeDistributeCmd::~HistoryOpTypeDistributeCmd(void)
{
}


bool HistoryOpTypeDistributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("HistoryOpTypeDistributeCmd ProcessCmd!");
	CStatClient::iterator s;
	Json::Value item, sample;
	if (!(InitStatInfo(in, prs) && VersionCheck(prs)))
		return false;

	s = m_stc[TTC_GET_COUNT];
	item["size"] = "select";
	item["num"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_ALL);
	sample.append(item);
	item.clear();
	s = m_stc[TTC_INSERT_COUNT];
	item["size"] = "insert";
	item["num"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_ALL);
	sample.append(item);
	item.clear();
	s = m_stc[TTC_UPDATE_COUNT];
	item["size"] = "update";
	item["num"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_ALL);
	sample.append(item);
	item.clear();
	s = m_stc[TTC_DELETE_COUNT];
	item["size"] = "delete";
	item["num"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_ALL);
	sample.append(item);
	item.clear();
	s = m_stc[TTC_PURGE_COUNT];
	item["size"] = "purge";
	item["num"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_ALL);
	sample.append(item);
	
	prs.resp["sample"] = sample;

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
