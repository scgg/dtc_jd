#include "CurrentLastAccDistributeCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentLastAccDistributeCmd)

CurrentLastAccDistributeCmd::CurrentLastAccDistributeCmd(void)
{
	m_ver = 40305;
}


CurrentLastAccDistributeCmd::~CurrentLastAccDistributeCmd(void)
{
}

int CurrentLastAccDistributeCmd::GetSize() {
	return m_mem->last_acc();
}


bool CurrentLastAccDistributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("CurrentLastAccDistributeCmd ProcessCmd!");
	if (!(InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs)))
		return false;
	log_debug("CurrentLastAccDistributeCmd::ProcessCmd init ok!");
	char buf[256];

	m_dis = {{0, 1, 0, PROBER_INTERVAL_TYPE_LC_RO}, {1, 4, 0, PROBER_INTERVAL_TYPE_LC_RO}, {4, 16, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {16, 24, 0, PROBER_INTERVAL_TYPE_LC_RO}, {24, 36, 0, PROBER_INTERVAL_TYPE_LC_RO}, {36, 48, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {48, 96, 0, PROBER_INTERVAL_TYPE_LC_RO}, {96, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};

	memset(buf, 0, sizeof(char) * 256);

	IterateMem();

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		if (m_dis[i][0] == m_dis[i][1])
			snprintf(buf, 255, "%d小时内", m_dis[i][0]);
		else
			snprintf(buf, 255, "%d-%d小时内", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		prs.resp["sample"].append(item);
	}

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
