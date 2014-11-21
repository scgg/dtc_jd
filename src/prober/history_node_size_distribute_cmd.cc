#include "history_node_size_distribute_cmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(HistoryNodeSizeDsitributeCmd)

HistoryNodeSizeDsitributeCmd::HistoryNodeSizeDsitributeCmd(void)
{
	m_ver = 40305;
}


HistoryNodeSizeDsitributeCmd::~HistoryNodeSizeDsitributeCmd(void)
{
}


bool HistoryNodeSizeDsitributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("HistoryNodeSizeDsitributeCmd ProcessCmd!");
	CStatClient::iterator s;
	if (!(InitStatInfo(in, prs) && VersionCheck(prs)))
		return false;

	s = m_stc[DATA_SIZE_HISTORY_STAT];

	Json::Value item;
	char buf[64];
	int64_t sc[16];
	int sn = m_stc.GetCountBase(s->id(), sc);
	int64_t all = m_stc.ReadSampleCounter(s, SCC_ALL);
	int64_t cur = m_stc.ReadSampleCounter(s, SCC_ALL, 1);
	snprintf(buf, 64, "0-%d", (int)sc[0]);
	item["size"] = buf;
	item["num"] = (Json::Value::Int64)(all - cur);
	prs.resp.append(item);
	
	for (int n = 2; n <= sn; ++n) {
		item.clear();
		all = cur;
		cur = m_stc.ReadSampleCounter(s, SCC_ALL, n);
		snprintf(buf, 64, "%d-%d", (int)sc[n - 2], (int)sc[n - 1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)(all - cur);
		prs.resp.append(item);
	}

	item.clear();
	snprintf(buf, 64, "%d-%d", (int)sc[sn - 1], INT_MAX);
	item["size"] = buf;
	item["num"] = (Json::Value::Int64)cur;
	prs.resp.append(item);

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
