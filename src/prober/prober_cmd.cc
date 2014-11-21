#include "prober_cmd.h"
#include "StatTTCDef.h"
#include "log.h"
#include "segvcatch.h"

extern CConfig* gConfig;
extern CDbConfig* gDbConfig;
extern CTableDefinition* gTableDef;

#define DISTRI_NUM 8

std::map<std::string, ClassInfo*> CProberCmd::m_classInfoMap;
CProberCmd::CProberCmd()
{
	m_mem = NULL;
	m_onlyDirty = false;
	m_noEmpty = false;
}


CProberCmd::~CProberCmd()
{
	if (NULL != m_mem)
		delete m_mem;
}

void CProberCmd::SetNoEmpty()
{
	m_noEmpty = true;
}

bool CProberCmd::ProcessCmd(Json::Value &in, ProberResultSet &prs) {
	return true;
}

void CProberCmd::Cal() {
	int row_size;
	for (int i = m_start; i < m_end; ++i) {
		CNode node;
		try {
			node = I_SEARCH(i);
		} catch (std::exception& e) {
			log_error("search node id seg fault");
			continue;
		}
		int size = 0;
		if (!node)
			continue;
		if (node.NotInLruList())
			continue;
		if (m_mem->IsTimeMarker(node))
			continue;
		if (m_onlyDirty && !node.IsDirty())
			continue;
		m_mem->set_node(node);
		try {
			row_size = m_mem->row_size();
		} catch (std::exception& e) {
			log_error("row_size seg fault");
			continue;
		}
		if (m_noEmpty && row_size == 0)
			continue;
		try {
			size = GetSize();
		} catch (std::exception& e) {
			log_error("GetSize seg fault");
			continue;
		}
		//log_debug("fetched size: %d", size);
		for (int i = 0; i < DISTRI_NUM; ++i) {
			bool flag = false;
			/* interval type:
			 * 	PROBER_INTERVAL_TYPE_LO_RO for (a, b)
			 * 	PROBER_INTERVAL_TYPE_LC_RO for [a, b)
			 * 	PROBER_INTERVAL_TYPE_LO_RC for (a, b]
			 * 	PROBER_INTERVAL_TYPE_LC_RC for [a, b]
			 */
			//log_debug("left point: %d, right point: %d, type: %d", m_dis[i][PROBER_INTERVAL_LEFT], m_dis[i][PROBER_INTERVAL_RIGHT], m_dis[i][PROBER_INTERVAL_TYPE]);
			switch (m_dis[i][PROBER_INTERVAL_TYPE]) {
			case PROBER_INTERVAL_TYPE_LO_RO:
				if (size > m_dis[i][PROBER_INTERVAL_LEFT] && size < m_dis[i][PROBER_INTERVAL_RIGHT])
					flag = true;
				break;
			case PROBER_INTERVAL_TYPE_LC_RO:
				if (size >= m_dis[i][PROBER_INTERVAL_LEFT] && size < m_dis[i][PROBER_INTERVAL_RIGHT])
					flag = true;
				break;
			case PROBER_INTERVAL_TYPE_LO_RC:
				if (size > m_dis[i][PROBER_INTERVAL_LEFT] && size <= m_dis[i][PROBER_INTERVAL_RIGHT])
					flag = true;
				break;
			case PROBER_INTERVAL_TYPE_LC_RC:
				if (size >= m_dis[i][PROBER_INTERVAL_LEFT] && size <= m_dis[i][PROBER_INTERVAL_RIGHT])
					flag = true;
				break;
			}
			if (flag) {
				++m_dis[i][PROBER_INTERVAL_COUNTER];
				//log_debug("key: %d, size: %d, counter: %d", *(int*)m_mem->key(), size, m_dis[i][PROBER_INTERVAL_COUNTER]);
				break;
			}
		}
		m_datas += size;
		if(0 == row_size)
			++m_empty;
		else
			++m_keys;
	}
	return;
}

void CProberCmd::AttachMem(CProberCmd *cmd) {
	m_mem = cmd->m_mem;
}

void CProberCmd::DetachMem() {
	m_mem = NULL;
}

void CProberCmd::SetInterval(int start, int end) {
	m_start = start;
	m_end = end;
}

bool CProberCmd::InitAndCheck(Json::Value &in, ProberResultSet &prs) {
	if (InitStatInfo(in, prs) && VersionCheck(prs) && InitMemInfo(in, prs))
		return true;
	return false;
}

bool CProberCmd::InitStatInfo(Json::Value &in, ProberResultSet &prs) {
	std::string stat = std::string("/usr/local/dtc/") + in["accesskey"].asString() + "/stat/ttc.stat.idx";
	int ret = m_stc.InitStatInfo("ttcd", stat.c_str());
	if (ret < 0) {
		log_error("Cannot Initialize StatInfo: %s", m_stc.ErrorMessage());
		prs.retcode = RET_CODE_INIT_STAT_ERR;
		prs.errmsg = "打开实例统计文件出错";
		return false;
	}
	m_stc.CheckPoint();
	return true;
}

bool CProberCmd::InitMemInfo(Json::Value &in, ProberResultSet &prs) {
	m_mem = new CCacheMemoryInfo;
	if (m_mem->cache_open()) {
		prs.retcode = RET_CODE_INIT_MEM_INFO_ERR;
		prs.errmsg = "打开缓存出错";
		log_error("open cache error, %s", in["accesskey"].asString().c_str());
		return false;
	}
	return true;
}

bool CProberCmd::VersionCheck(ProberResultSet &prs) {
	CStatClient::iterator s = m_stc[S_VERSION];
	int ver = (int)m_stc.ReadCounterValue(s, SC_CUR);
	log_debug("Current dtcd: %d, needed dtcd: %d", ver, m_ver);
	if (ver < m_ver) {
		char c_buf[16];
		char m_buf[16];
		log_error("Current dtcd do not support this cmd");
		prs.retcode = RET_CODE_CMD_NOT_SUPPORT;
		snprintf(c_buf, 16, "%d.%d.%d", ver/10000, ver/100%100, ver%100);
		snprintf(m_buf, 16, "%d.%d.%d", m_ver/10000, m_ver/100%100, m_ver%100);
		prs.errmsg = std::string("当前版本DTC不支持此命令。当前版本为") + c_buf + "，最低支持版本为" + m_buf;
		return false;
	}
	return true;
}

void CProberCmd::Combine(CProberCmd *cmd) {
	m_datas += cmd->m_datas;
	m_empty += cmd->m_empty;
	m_keys += cmd->m_keys;
	for (int i = 0; i < DISTRI_NUM; ++i)
		m_dis[i][PROBER_INTERVAL_COUNTER] += cmd->m_dis[i][PROBER_INTERVAL_COUNTER];
}

bool CProberCmd::AsyncCheck(ProberResultSet &prs) {
	int async = gConfig->GetIntVal("cache", "DelayUpdate", 0);
	if (async == 0) {
		prs.retcode = RET_CODE_CMD_NOT_SUPPORT;
		prs.errmsg = std::string("当前DTC实例非异步模式，不支持此命令");
		return false;
	}
	return true;
}

int CProberCmd::GetSize() {
	return 0;
}

bool CProberCmd::Register(ClassInfo* classInfo)
{
	
	m_classInfoMap.insert(std::make_pair(classInfo->strClassName, classInfo));
	return true;
}


CProberCmd* CProberCmd::CreatConcreteCmd(const std::string& className)
{
	log_debug("CProberCmd::CreatConcreteCmd start");
	std::map<std::string, ClassInfo*>::const_iterator iter = m_classInfoMap.find(className);
	if (m_classInfoMap.end() != iter)
	{
	if(NULL != iter->second)
	{
		if (NULL != iter->second->fun)
		{
			log_debug("CProberCmd::CreatConcreteCmd success");
			return iter->second->fun();
		}

	}
	}

	log_debug("CProberCmd::CreatConcreteCmd fail, return NULL");
	return NULL;
}
