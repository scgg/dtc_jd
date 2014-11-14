#include "CurrentDirtyDistributeCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentDirtyDistributeCmd)

CurrentDirtyDistributeCmd::CurrentDirtyDistributeCmd(void)
{
	m_ver = 40305;
}


CurrentDirtyDistributeCmd::~CurrentDirtyDistributeCmd(void)
{
}

int CurrentDirtyDistributeCmd::GetSize() {
	return m_mem->last_upd();
}


bool CurrentDirtyDistributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("CurrentDirtyDistributeCmd ProcessCmd!");
	if (!(AsyncCheck(prs) && InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs)))
		return false;
	log_debug("CurrentDirtyDistributeCmd::ProcessCmd init ok!");
	char buf[256];

	m_dis = {{0, 1, 0, PROBER_INTERVAL_TYPE_LC_RO}, {1, 4, 0, PROBER_INTERVAL_TYPE_LC_RO}, {4, 8, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {8, 16, 0, PROBER_INTERVAL_TYPE_LC_RO}, {16, 24, 0, PROBER_INTERVAL_TYPE_LC_RO}, {24, 48, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {48, 72, 0, PROBER_INTERVAL_TYPE_LC_RO}, {72, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};

	memset(buf, 0, sizeof(char) * 256);

	IterateMem(PROBER_ITERATE_DIRTY);

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		snprintf(buf, 255, "%d-%d小时", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		prs.resp["sample"].append(item);
	}

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
