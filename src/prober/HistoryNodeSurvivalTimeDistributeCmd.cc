#include "HistoryNodeSurvivalTimeDistributeCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(HistoryNodeSurvivalTimeDistributeCmd)

HistoryNodeSurvivalTimeDistributeCmd::HistoryNodeSurvivalTimeDistributeCmd(void)
{
	m_ver = 40305;
}


HistoryNodeSurvivalTimeDistributeCmd::~HistoryNodeSurvivalTimeDistributeCmd(void)
{
}


bool HistoryNodeSurvivalTimeDistributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("HistoryNodeSurvivalTimeDistributeCmd ProcessCmd!");
	CStatClient::iterator s;
	if (!(InitStatInfo(in, prs) && VersionCheck(prs)))
		return false;

	s = m_stc[DATA_SURVIVAL_HOUR_STAT];

	Json::Value sample;
	Json::Value item;
	char buf[64];
	int64_t sc[16];
	int sn = m_stc.GetCountBase(s->id(), sc);
	int64_t all = m_stc.ReadSampleCounter(s, SCC_ALL);
	int64_t cur = m_stc.ReadSampleCounter(s, SCC_ALL, 1);
	snprintf(buf, 64, "0-%d小时", (int)sc[0]);
	item["size"] = buf;
	item["num"] = (Json::Value::Int64)(all - cur);
	sample.append(item);
	
	for (int n = 2; n <= sn; ++n) {
		item.clear();
		all = cur;
		cur = m_stc.ReadSampleCounter(s, SCC_ALL, n);
		snprintf(buf, 64, "%d-%d小时", (int)sc[n - 2], (int)sc[n - 1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)(all - cur);
		sample.append(item);
	}

	item.clear();
	snprintf(buf, 64, "%d-%d小时", (int)sc[sn - 1], INT_MAX);
	item["size"] = buf;
	item["num"] = (Json::Value::Int64)cur;
	sample.append(item);

	prs.resp["sample"] = sample;
	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
