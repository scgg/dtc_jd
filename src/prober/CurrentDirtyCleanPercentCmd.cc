#include "CurrentDirtyCleanPercentCmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentDirtyCleanPercentCmd)

CurrentDirtyCleanPercentCmd::CurrentDirtyCleanPercentCmd(void)
{
	m_ver = 40305;
}


CurrentDirtyCleanPercentCmd::~CurrentDirtyCleanPercentCmd(void)
{
}


bool CurrentDirtyCleanPercentCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs)
{
	log_debug("CurrentDirtyCleanPercentCmd ProcessCmd!");
	if (!(AsyncCheck(prs) && InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs)))
		return false;
	log_debug("CurrentDirtyCleanPercentCmd::ProcessCmd init ok!");

	IterateMem();

	Json::Value item;
	item["size"] = "脏节点数";
	item["num"] = (Json::Value::Int64)m_mem->total_dirty_node();
	prs.resp["sample"].append(item);
	item.clear();
	item["size"] = "干净节点数";
	item["num"] = (Json::Value::Int64)m_datas - m_mem->total_dirty_node();
	prs.resp["sample"].append(item);
	
	prs.errmsg = "ok";
	prs.retcode = 0;

	return true;
}
