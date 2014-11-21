#include "memory_usage_cmd.h"
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CMemoryUsageCmd)

CMemoryUsageCmd::CMemoryUsageCmd(void)
{
	m_ver = 40304;
}


CMemoryUsageCmd::~CMemoryUsageCmd(void)
{
}


bool CMemoryUsageCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("MemoryUsageCmd ProcessCmd!");
	CStatClient::iterator s;
	if (!(InitStatInfo(in, prs) && VersionCheck(prs)))
		return false;
	s = m_stc[TTC_CACHE_SIZE];
	prs.resp["memtotalsize"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_10S);
	s = m_stc[TTC_DATA_SIZE];
	prs.resp["useddatasize"] = (Json::Value::Int64)m_stc.ReadCounterValue(s, SCC_10S);
	prs.retcode = RET_CODE_DONE;
	prs.errmsg = "ok";
	return true;
}
