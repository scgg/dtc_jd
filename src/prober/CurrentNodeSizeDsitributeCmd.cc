#include "CurrentNodeSizeDsitributeCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentNodeSizeDsitributeCmd)

CurrentNodeSizeDsitributeCmd::CurrentNodeSizeDsitributeCmd(void)
{
	m_ver = 40304;
}


CurrentNodeSizeDsitributeCmd::~CurrentNodeSizeDsitributeCmd(void)
{
}


int CurrentNodeSizeDsitributeCmd::GetSize() {
	return m_mem->data_size();
}


bool CurrentNodeSizeDsitributeCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("CurrentNodeSizeDsitributeCmd ProcessCmd!");
	if (!(InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs)))
		return false;
	log_debug("CurrentNodeSizeDsitributeCmd::ProcessCmd init ok!");
	char buf[256];

	m_dis = {{0, 64, 0, PROBER_INTERVAL_TYPE_LC_RO}, {64, 128, 0, PROBER_INTERVAL_TYPE_LC_RO}, {128, 256, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {256, 512, 0, PROBER_INTERVAL_TYPE_LC_RO}, {512, 1024, 0, PROBER_INTERVAL_TYPE_LC_RO}, {1024, 2048, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {2048, 4096, 0, PROBER_INTERVAL_TYPE_LC_RO}, {4096, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};

	memset(buf, 0, sizeof(char) * 256);

	IterateMem();

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		snprintf(buf, 255, "%d-%d", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		prs.resp["sample"].append(item);
	}
	Json::Value array, elem;
	elem.append("数据大小总数(Bytes)");
	elem.append(m_datas);
	array.append(elem);
	elem.clear();
	elem.append("key的总数");
	elem.append(m_keys);
	array.append(elem);
	prs.resp["tables"] = array;

	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
