#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task_pkey.h"
#include "cache_writer.h"
#include "bin_malloc.h"
#include "sys_malloc.h"
#include "log.h"

cCacheWriter::cCacheWriter(void) : CCachePool(NULL)
{
	pstItem = NULL;
	iRowIdx = 0;
	iIsFull = 0;
	memset(achPackKey, 0, sizeof(achPackKey));
}

cCacheWriter::~cCacheWriter(void)
{
	if(pstItem != NULL)
		delete pstItem;
	pstItem = NULL;
}

int cCacheWriter::cache_open(CacheInfo* pstInfo, CTableDefinition* pstTab)
{
	int iRet;
	
	iRet = CCachePool::CacheOpen(pstInfo);
	if(iRet != E_OK){
		log_error("cache open error: %d, %s", iRet, Error());
		return -1;
	}
	
	pstItem = new CRawData(&g_stSysMalloc, 1);
	if(pstItem == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "new CRawData error: %m");
		return -2;
	}
	
	CUpdateMode stUpdateMod;
	stUpdateMod.m_iAsyncServer = pstInfo->syncUpdate?MODE_SYNC:MODE_ASYNC;
	stUpdateMod.m_iUpdateMode = pstInfo->syncUpdate?MODE_SYNC:MODE_ASYNC;
	stUpdateMod.m_iInsertMode = pstInfo->syncUpdate?MODE_SYNC:MODE_ASYNC;
	stUpdateMod.m_uchInsertOrder = 0;
	
	if(pstTab->IndexFields() > 0)
	{
#if HAS_TREE_DATA
		pstDataProcess = new CTreeDataProcess(CBinMalloc::Instance(), pstTab, this, &stUpdateMod);
#else
		log_error("tree index not supported, index field num[%d]", pstTab->IndexFields());
		return -1;
#endif
	}
	else
		pstDataProcess = new CRawDataProcess(CBinMalloc::Instance(), pstTab, this, &stUpdateMod);
	if(pstDataProcess == NULL){
		log_error("create %s error: %m", pstTab->IndexFields()>0?"CTreeDataProcess":"CRawDataProcess");
		return -3;
	}
	
	return 0;
}

int cCacheWriter::begin_write()
{
	iRowIdx = 0;
	
	return 0;
}

int cCacheWriter::full()
{
	return(iIsFull);
}

int cCacheWriter::AllocNode(const CRowValue& row)
{
	int iRet;
	
	iRet = CTaskPackedKey::BuildPackedKey(row.TableDefinition(), row.FieldValue(0), sizeof(achPackKey), achPackKey);
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "build packed key error: %d", iRet);
		return -1;
	}
	
	stCurNode = CacheAllocate(achPackKey);
	if(!stCurNode){
		snprintf(szErrMsg, sizeof(szErrMsg), "cache alloc node error");
		iIsFull = 1;
		return -2;
	}
	
	iRet = pstItem->Init(row.TableDefinition()->KeyFields()-1, row.TableDefinition()->KeyFormat(), achPackKey, 0);
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "raw data init error: %s", pstItem->GetErrMsg());
		CachePurge(achPackKey);
		return -3;
	}
	
	return 0;
}

int cCacheWriter::write_row(const CRowValue& row)
{
	int iRet;
	
	if(iRowIdx == 0){
		if(AllocNode(row) != 0)
			return -1;
	}
	
	iRet = pstItem->InsertRow(row, false, false);
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "insert row error: %s", pstItem->GetErrMsg());
		CachePurge(achPackKey);
		return -2;
	}
	
	iRowIdx++;
	return 0;
}

int cCacheWriter::commit_node()
{
	int iRet;
	
	if(iRowIdx < 1)
		return 0;
	
	const CMemHead* pstHead = CBinMalloc::Instance()->GetHeadInfo();
	if(pstHead->m_hTop+pstItem->DataSize()+MINSIZE >= pstHead->m_tSize){
		iIsFull = 1;
		CachePurge(achPackKey);
		return -1;
	}
	
	iRet = pstDataProcess->ReplaceData(&stCurNode, pstItem);
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "write data into cache error");
		CachePurge(achPackKey);
		return -2;
	}
	
	iRowIdx = 0;
	memset(achPackKey, 0, sizeof(achPackKey));
 	pstItem->Destroy();	
	return 0;
}

int cCacheWriter::rollback_node()
{
	pstItem->Destroy();
	CachePurge(achPackKey);
	memset(achPackKey, 0, sizeof(achPackKey));
	
	return 0;
}

