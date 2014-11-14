#include "CurrentRowSizeDsitributeCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentRowSizeDsitributeCmd)

CurrentRowSizeDsitributeCmd::CurrentRowSizeDsitributeCmd(void)
{
	m_ver = 40304;
}


CurrentRowSizeDsitributeCmd::~CurrentRowSizeDsitributeCmd(void)
{
}


int CurrentRowSizeDsitributeCmd::GetSize() {
	return m_mem->row_size();
}


bool CurrentRowSizeDsitributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("CurrentRowSizeDsitributeCmd ProcessCmd!");
	if (!(InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs)))
		return false;
	log_debug("CurrentRowSizeDsitributeCmd::ProcessCmd init ok!");
	char buf[256];

	m_dis = {{1, 1, 0, PROBER_INTERVAL_TYPE_LC_RC}, {2, 2, 0, PROBER_INTERVAL_TYPE_LC_RC}, {3, 3, 0, PROBER_INTERVAL_TYPE_LC_RC},
		 {4, 4, 0, PROBER_INTERVAL_TYPE_LC_RC}, {4, 8, 0, PROBER_INTERVAL_TYPE_LO_RC}, {8, 16, 0, PROBER_INTERVAL_TYPE_LO_RC},
		 {16, 32, 0, PROBER_INTERVAL_TYPE_LO_RC}, {32, INT_MAX, 0, PROBER_INTERVAL_TYPE_LO_RC}};

	memset(buf, 0, sizeof(char) * 256);

	IterateMem();

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		if (m_dis[i][0] == m_dis[i][1])
			snprintf(buf, 255, "%d", m_dis[i][0]);
		else
			snprintf(buf, 255, "%d-%d", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		prs.resp["sample"].append(item);
	}
	Json::Value array, elem;
	elem.append("行总数");
	elem.append(m_datas);
	array.append(elem);
	elem.clear();
	elem.append("非空key的总数");
	elem.append(m_keys);
	array.append(elem);
	elem.clear();
	elem.append("空节点总数");
	elem.append(m_empty);
	array.append(elem);
	elem.clear();
	elem.append("节点总数");
	elem.append(m_keys + m_empty);
	array.append(elem);
	prs.resp["tables"] = array;

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
