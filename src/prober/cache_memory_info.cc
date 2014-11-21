#include "cache_memory_info.h"
#include "sys_malloc.h"
#include "RelativeHourCalculator.h"

extern CConfig* gConfig;
extern CTableDefinition* gTableDef;

CCacheMemoryInfo::CCacheMemoryInfo() : CCachePool(NULL)
{
}

CCacheMemoryInfo::~CCacheMemoryInfo()
{
}

int CCacheMemoryInfo::cache_open()
{
	log_debug("CCacheMemoryInfo::cache_open start");
	CacheInfo info;
	memset(&info, 0, sizeof(info));
	const char* keyStr = gConfig->GetStrVal("cache", "CacheShmKey");
	if (!isdigit(keyStr[0])) {
		log_error("parse cache shm key error.");
		return -1;
	}
	info.ipcMemKey = strtol(keyStr, NULL, 0);
	info.keySize = gTableDef->KeyFormat();
	info.readOnly = 1;
	log_debug("CCachePool::CacheOpen for key %d, key size %d", info.ipcMemKey, info.keySize);
	if (E_OK != CCachePool::CacheOpen(&info)) {
		log_error("CCachePool::CacheOpen error");
		return -1;
	}

	m_RelativeHour = RELATIVE_HOUR_CALCULATOR->GetRelativeHour();

	log_debug("CCacheMemoryInfo::cache_open end");

	return 0;
}

bool CCacheMemoryInfo::set_node(CNode &node)
{
	m_stCurNode = node;
	m_pstRawFormatItem = (CRawFormat*)(CBinMalloc::Instance()->Handle2Ptr(m_stCurNode.VDHandle()));
	return true;
}

int CCacheMemoryInfo::data_size()
{
	return m_pstRawFormatItem->m_uiDataSize;
}

int CCacheMemoryInfo::row_size()
{
	return m_pstRawFormatItem->m_uiRowCnt;
}

int CCacheMemoryInfo::get_cout()
{
	return m_pstRawFormatItem->m_uchGetCount;
}

int CCacheMemoryInfo::last_acc()
{
	return m_RelativeHour - m_pstRawFormatItem->m_LastAccessHour;
}

int CCacheMemoryInfo::last_upd()
{
	return m_RelativeHour - m_pstRawFormatItem->m_LastUpdateHour;
}

int CCacheMemoryInfo::survival()
{
	log_debug("cur hour: %ld, node create hour: %d", m_RelativeHour, m_pstRawFormatItem->m_CreateHour);
	return m_RelativeHour - m_pstRawFormatItem->m_CreateHour;
}

char* CCacheMemoryInfo::key()
{
	return m_pstRawFormatItem->m_achKey;
}

int CCacheMemoryInfo::total_dirty_node()
{
	return _ngInfo->TotalDirtyNode();
}

int CCacheMemoryInfo::total_used_node()
{
	return _ngInfo->TotalUsedNode();
}
