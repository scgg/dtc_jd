#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "task_pkey.h"
#include "cache_reader.h"
#include "log.h"
#include "sys_malloc.h"

cCacheReader::cCacheReader(void) : CCachePool(NULL)
{
	pstItem = NULL;
	pstDataProcess = NULL;
	iInDirtyLRU = 1;
	notFetch = 1;
}

cCacheReader::~cCacheReader(void)
{
	if(pstItem != NULL)
		delete pstItem;
	pstItem = NULL;
}

int cCacheReader::cache_open(int shmKey, int keySize, CTableDefinition* pstTab)
{
	int iRet;
	
	CacheInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.ipcMemKey = shmKey;
	stInfo.keySize = keySize;
	stInfo.readOnly = 1;
	
	iRet = CCachePool::CacheOpen(&stInfo);
	if(iRet != E_OK)
		return -1;
	
	pstItem = new CRawData(&g_stSysMalloc, 1);
	if(pstItem == NULL){
		snprintf(error_message, sizeof(error_message), "new CRawData error: %m");
		return -1;
	}
	
	CUpdateMode stUpdateMod;
	stUpdateMod.m_iAsyncServer = MODE_SYNC;
	stUpdateMod.m_iUpdateMode = MODE_SYNC;
	stUpdateMod.m_iInsertMode = MODE_SYNC;
	stUpdateMod.m_uchInsertOrder = 0;
	
	if(pstTab->IndexFields() > 0) {
#if HAS_TREE_DATA
		pstDataProcess = new CTreeDataProcess(CBinMalloc::Instance(), pstTab, this, &stUpdateMod);
#else
		log_error("tree index not supported, index field num[%d]", pstTab->IndexFields());
		return -1;
#endif
	} else
		pstDataProcess = new CRawDataProcess(CBinMalloc::Instance(), pstTab, this, &stUpdateMod);
	if(pstDataProcess == NULL){
		log_error("create %s error: %m", pstTab->IndexFields()>0?"CTreeDataProcess":"CRawDataProcess");
		return -1;
	}
	
	return 0;
}

int cCacheReader::begin_read()
{
	stDirtyHead = DirtyLruHead();
	stClrHead = CleanLruHead();
	if(!DirtyLruEmpty()){
		iInDirtyLRU = 1;
		stCurNode = stDirtyHead;
	}
	else{
		iInDirtyLRU = 0;
		stCurNode = stClrHead;
	}
	return 0;
}

int cCacheReader::fetch_node()
{

     pstItem->Destroy();
	if(!stCurNode){
		snprintf(error_message, sizeof(error_message), "begin read first!");
		return -1;
	}
	if(end()){
		snprintf(error_message, sizeof(error_message), "reach end of cache");
		return -2;
	}
	notFetch = 0;
	
	curRowIdx = 0;
	if(iInDirtyLRU){
		while(stCurNode != stDirtyHead && IsTimeMarker(stCurNode))
			stCurNode = stCurNode.Next();
		if(stCurNode != stDirtyHead && !IsTimeMarker(stCurNode)){
			if(pstDataProcess->GetAllRows(&stCurNode, pstItem) != 0){
				snprintf(error_message, sizeof(error_message), "get node's data error");
				return -3;
			}
			return(0);
		}
		
		iInDirtyLRU = 0;
		stCurNode = stClrHead.Next();
	}
	
	stCurNode = stCurNode.Next();
	if(stCurNode != stClrHead){
		if(pstDataProcess->GetAllRows(&stCurNode, pstItem) != 0){
			snprintf(error_message, sizeof(error_message), "get node's data error");
			return -3;
		}
	}
	else{
		snprintf(error_message, sizeof(error_message), "reach end of cache");
		return -2;
	}
	
	return(0);
}

int cCacheReader::num_rows()
{
	if(pstItem == NULL)
		return(-1);
	
	return pstItem->TotalRows();
}

int cCacheReader::read_row(CRowValue& row)
{
	while(notFetch || curRowIdx >= (int)pstItem->TotalRows()){
		if(fetch_node() != 0)
			return -1;
	}
	
	CTaskPackedKey::UnpackKey(row.TableDefinition(), pstItem->Key(), row.FieldValue(0)); 
	
	if(pstItem->DecodeRow(row, uchRowFlags, 0) != 0)
		return -2;
	
	curRowIdx++;
	
	return 0;
}

