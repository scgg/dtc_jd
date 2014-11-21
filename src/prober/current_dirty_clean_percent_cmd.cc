#include "current_dirty_clean_percent_cmd.h"
#include <limits.h>
#include "StatTTCDef.h"
#include "log.h"

DEFINE_REG_CALSS_INFO(CurrentDirtyCleanPercentCmd)

CurrentDirtyCleanPercentCmd::CurrentDirtyCleanPercentCmd(void)
{
	m_ver = 40305;
	m_onlyDirty = true;
}


CurrentDirtyCleanPercentCmd::~CurrentDirtyCleanPercentCmd(void)
{
}

bool CurrentDirtyCleanPercentCmd::InitAndCheck(Json::Value &in, ProberResultSet &prs)
{
	if (AsyncCheck(prs) && CProberCmd::InitAndCheck(in, prs))
		return true;
	return false;
}

bool CurrentDirtyCleanPercentCmd::ReplyBody(std::string &respBody)
{
	Json::Value item;
	Json::Value resp, out;
	Json::FastWriter writer;

	item["size"] = "脏节点数";
	item["num"] = (Json::Value::Int64)m_mem->total_dirty_node();
	resp["sample"].append(item);
	item.clear();
	item["size"] = "干净节点数";
	item["num"] = (Json::Value::Int64)m_datas - m_mem->total_dirty_node();
	resp["sample"].append(item);
	
	out["resp"] = resp;
	out["retcode"] = 0;
	out["errmsg"] = "ok";
	respBody = writer.write(out);

	return true;
}
