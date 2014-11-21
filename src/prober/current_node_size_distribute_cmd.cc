#include "current_node_size_distribute_cmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentNodeSizeDsitributeCmd)

CurrentNodeSizeDsitributeCmd::CurrentNodeSizeDsitributeCmd(void)
{
	m_dis = {{0, 64, 0, PROBER_INTERVAL_TYPE_LC_RO}, {64, 128, 0, PROBER_INTERVAL_TYPE_LC_RO}, {128, 256, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {256, 512, 0, PROBER_INTERVAL_TYPE_LC_RO}, {512, 1024, 0, PROBER_INTERVAL_TYPE_LC_RO}, {1024, 2048, 0, PROBER_INTERVAL_TYPE_LC_RO},
		 {2048, 4096, 0, PROBER_INTERVAL_TYPE_LC_RO}, {4096, INT_MAX, 0, PROBER_INTERVAL_TYPE_LC_RO}};
	m_ver = 40304;
}

CurrentNodeSizeDsitributeCmd::~CurrentNodeSizeDsitributeCmd(void)
{
}

int CurrentNodeSizeDsitributeCmd::GetSize() {
	return m_mem->data_size();
}

bool CurrentNodeSizeDsitributeCmd::ReplyBody(std::string &respBody)
{
	char buf[256];
	Json::Value resp, out;
	Json::FastWriter writer;
	memset(buf, 0, sizeof(char) * 256);

	for (int i = 0; i < DISTRI_NUM; ++i) {
		Json::Value item;
		snprintf(buf, 255, "%d-%d", m_dis[i][0], m_dis[i][1]);
		item["size"] = buf;
		item["num"] = (Json::Value::Int64)m_dis[i][2];
		resp["sample"].append(item);
	}

	Json::Value array, elem;
	elem.append("数据大小总数(Bytes)");
	elem.append(m_datas);
	array.append(elem);
	elem.clear();
	elem.append("key的总数");
	elem.append(m_keys);
	array.append(elem);
	resp["tables"] = array;

	out["resp"] = resp;
	out["retcode"] = 0;
	out["errmsg"] = "ok";
	respBody = writer.write(out);

	return true;
}
