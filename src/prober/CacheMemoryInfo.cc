#include "CacheMemoryInfo.h"
#include "sys_malloc.h"
#include "RelativeHourCalculator.h"

extern CConfig* gConfig;
extern CTableDefinition* gTableDef;

CCacheMemoryInfo::CCacheMemoryInfo() : CCachePool(NULL)
{
}

CCacheMemoryInfo::~CCacheMemoryInfo()
{
	if (m_pstItem != NULL) {
		m_pstItem->Destroy();
		delete m_pstItem;
	}
	m_pstItem = NULL;
	if (m_pstDataProcess != NULL)
		delete m_pstDataProcess;
	m_pstDataProcess = NULL;
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
	if (NULL == (m_pstItem = new CRawData(&g_stSysMalloc, 1))) {
		log_error("new CRawData error");
		return -1;
	}

	CUpdateMode updateMod;
	updateMod.m_iAsyncServer = MODE_SYNC;
	updateMod.m_iUpdateMode = MODE_SYNC;
	updateMod.m_iInsertMode = MODE_SYNC;
	updateMod.m_uchInsertOrder = 0;

	m_pstDataProcess = new CRawDataProcess(CBinMalloc::Instance(), gTableDef, this, &updateMod);
	if (NULL == m_pstDataProcess) {
		log_error("create CRawDataProcess error");
		return -1;
	}

	m_stCleanHead = CleanLruHead();
	m_stDirtyHead = DirtyLruHead();
	m_stEmptyHead = EmptyLruHead();

	log_debug("CCacheMemoryInfo::cache_open end");

	return 0;
}

int CCacheMemoryInfo::begin_clean()
{
	if (NODE_LIST_EMPTY(m_stCleanHead))
		m_stCurNode = m_stCleanHead;
	else
		m_stCurNode = m_stCleanHead.Next();
	log_debug("m_stCurNode: %d, m_stCleanHead: %d", m_stCurNode.NodeID(), m_stCleanHead.NodeID());
	return 0;
}

int CCacheMemoryInfo::begin_empty()
{
	if (NODE_LIST_EMPTY(m_stEmptyHead))
		m_stCurNode = m_stEmptyHead;
	else
		m_stCurNode = m_stEmptyHead.Next();
	log_debug("m_stCurNode: %d, m_stEmptyHead: %d", m_stCurNode.NodeID(), m_stEmptyHead.NodeID());
	return 0;
}

int CCacheMemoryInfo::begin_dirty()
{
	if (NODE_LIST_EMPTY(m_stDirtyHead))
		m_stCurNode = m_stDirtyHead;
	else
		m_stCurNode = m_stDirtyHead.Next();
	log_debug("m_stCurNode: %d, m_stDirtyHead: %d", m_stCurNode.NodeID(), m_stDirtyHead.NodeID());
	return 0;
}


int CCacheMemoryInfo::fetch_node()
{
	m_pstItem->Destroy();
	if (!m_stCurNode) {
		log_error("begin read first!");
		return -1;
	}

	while (m_stCurNode != m_stDirtyHead && IsTimeMarker(m_stCurNode)) {
		log_debug("is time marker node id: %d", m_stCurNode.NodeID());
		m_stCurNode = m_stCurNode.Next();
	}
	if (!end()) {
		log_debug("fetched node id: %d", m_stCurNode.NodeID());
		m_pstRawFormatItem = (CRawFormat*)(CBinMalloc::Instance()->Handle2Ptr(m_stCurNode.VDHandle()));
		m_stCurNode = m_stCurNode.Next();
	} else {
		log_warning("reach end of cache");
		return -2;
	}

	return 0;
}

int CCacheMemoryInfo::data_size()
{
	//return m_pstItem->DataSize();
	return m_pstRawFormatItem->m_uiDataSize;
}

int CCacheMemoryInfo::row_size()
{
	//return m_pstItem->TotalRows();
	return m_pstRawFormatItem->m_uiRowCnt;
}

int CCacheMemoryInfo::get_cout()
{
	//return m_pstItem->GetSelectOpCount();
	return m_pstRawFormatItem->m_uchGetCount;
}

int CCacheMemoryInfo::last_acc()
{
	//log_debug("rh: %d, la: %d", (int)RELATIVE_HOUR_CALCULATOR->GetRelativeHour(), m_pstItem->GetLastAccessTimeByHour());
	//return RELATIVE_HOUR_CALCULATOR->GetRelativeHour() - m_pstItem->GetLastAccessTimeByHour();
	return RELATIVE_HOUR_CALCULATOR->GetRelativeHour() - m_pstRawFormatItem->m_LastAccessHour;
}

int CCacheMemoryInfo::last_upd()
{
	//log_debug("rh: %d, la: %d", (int)RELATIVE_HOUR_CALCULATOR->GetRelativeHour(), m_pstItem->GetLastAccessTimeByHour());
	//return RELATIVE_HOUR_CALCULATOR->GetRelativeHour() - m_pstItem->GetLastUpdateTimeByHour();
	return RELATIVE_HOUR_CALCULATOR->GetRelativeHour() - m_pstRawFormatItem->m_LastUpdateHour;
}

char* CCacheMemoryInfo::key()
{
	//return m_pstItem->Key();
	return m_pstRawFormatItem->m_achKey;
}

int CCacheMemoryInfo::total_dirty_node() {
	return _ngInfo->TotalDirtyNode();
}

int CCacheMemoryInfo::total_used_node() {
	return _ngInfo->TotalUsedNode();
}

bool CCacheMemoryInfo::end()
{
	return m_stCurNode == m_stCleanHead || m_stCurNode == m_stDirtyHead || m_stCurNode == m_stEmptyHead;
}
