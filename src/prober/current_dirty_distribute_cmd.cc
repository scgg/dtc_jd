#include "current_dirty_distribute_cmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentDirtyDistributeCmd)

CurrentDirtyDistributeCmd::CurrentDirtyDistributeCmd(void)
{
	m_ver = 40305;
	m_dis = {{0, 1, 0, PROBER_INTERVAL_TYPE_LC_RO}, {1, 4, 0, PROBER_INTERVAL_TYPE_LC_RO}, {4, 8, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {8, 16, 0, PROBER_INTERVAL_TYPE_LC_RO}, {16, 24, 0, PROBER_INTERVAL_TYPE_LC_RO}, {24, 48, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {48, 72, 0, PROBER_INTERVAL_TYPE_LC_RO}, {72, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};
	m_onlyDirty = true;
}


CurrentDirtyDistributeCmd::~CurrentDirtyDistributeCmd(void)
{
}

bool CurrentDirtyDistributeCmd::InitAndCheck(Json::Value &in, ProberResultSet &prs)
{
	if (AsyncCheck(prs) && CProberCmd::InitAndCheck(in, prs))
		return true;
	return false;
}

int CurrentDirtyDistributeCmd::GetSize()
{
	return m_mem->last_upd();
}

bool CurrentDirtyDistributeCmd::ReplyBody(std::string &respBody)
{
	Json::Value item;
	Json::Value resp, out;
	Json::FastWriter writer;
	char buf[256];

	memset(buf, 0, sizeof(char) * 256);

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		snprintf(buf, 255, "%d-%d小时", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		resp["sample"].append(item);
	}

	out["resp"] = resp;
	out["retcode"] = 0;
	out["errmsg"] = "ok";
	respBody = writer.write(out);

	return true;
}
