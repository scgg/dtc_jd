#include "current_get_count_distribute_cmd.h"
#include <limits.h>
#include <stdexcept>
#include "StatTTCDef.h"
#include "log.h"
#include "json/json.h"

DEFINE_REG_CALSS_INFO(CurrentGetCountDistributeCmd)

CurrentGetCountDistributeCmd::CurrentGetCountDistributeCmd(void)
{
	m_dis = {{1, 1, 0, PROBER_INTERVAL_TYPE_LC_RC}, {2, 2, 0, PROBER_INTERVAL_TYPE_LC_RC}, {2, 4, 0, PROBER_INTERVAL_TYPE_LO_RC},
		 {4, 8, 0, PROBER_INTERVAL_TYPE_LO_RC}, {8, 16, 0, PROBER_INTERVAL_TYPE_LO_RC}, {16, 32, 0, PROBER_INTERVAL_TYPE_LO_RC},
		 {32, 255, 0, PROBER_INTERVAL_TYPE_LO_RO}, {255, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};
	m_ver = 40305;
}

CurrentGetCountDistributeCmd::~CurrentGetCountDistributeCmd(void)
{
}

int CurrentGetCountDistributeCmd::GetSize()
{
	return m_mem->get_cout();
}

bool CurrentGetCountDistributeCmd::ReplyBody(std::string &respBody)
{
	char buf[256];
	Json::Value resp, out;
	Json::FastWriter writer;
	memset(buf, 0, sizeof(char) * 256);

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		if (m_dis[i][0] == m_dis[i][1])
			snprintf(buf, 255, "%d", m_dis[i][0]);
		else
			snprintf(buf, 255, "%d-%d", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		resp["sample"].append(item);
	}

	Json::Value array, elem;
	elem.append("Get总数");
	elem.append(m_datas);
	array.append(elem);
	elem.clear();
	elem.append("key的总数");
	elem.append(m_keys);
	array.append(elem);
	elem.clear();
	elem.append("空节点总数");
	elem.append(m_empty);
	array.append(elem);
	resp["tables"] = array;

	out["resp"] = resp;
	out["retcode"] = 0;
	out["errmsg"] = "ok";
	respBody = writer.write(out);

	return true;
}
