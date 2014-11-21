/************************************************************
Description:   封装了平板数据处理的各种接口
Version:         TTC 3.0
Function List:   
1.  GetAllRows()	查询所有行
2.  DeleteData()	根据task删除数据
3. GetData() 		查询数据
4. AppendData()	增加一行数据
5. ReplaceData()	用task的数据替换cache里的数据
6. UpdateData()	更新数据
7. FlushData()		将脏数据flush到db
8.PurgeData()		将cache里的数据删除
History:        
Paul    2008.07.01     3.0         build this moudle  
 ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raw_data_process.h"
#include "global.h"
#include "log.h"
#include "sys_malloc.h"

TTC_USING_NAMESPACE

CRawDataProcess::CRawDataProcess(CMallocator* pstMalloc, CTableDefinition* pstTab, CCachePool* pstPool, const CUpdateMode* pstUpdateMode): 
	m_stRawData(pstMalloc), m_pstTab(pstTab), m_pMallocator(pstMalloc), m_pstPool(pstPool)
{
	memcpy(&m_stUpdateMode, pstUpdateMode, sizeof(m_stUpdateMode));
	nodeSizeLimit = 0;
	history_datasize =  statmgr.GetSample(DATA_SIZE_HISTORY_STAT);
	history_rowsize = statmgr.GetSample(ROW_SIZE_HISTORY_STAT);
	
}

CRawDataProcess::~CRawDataProcess()
{

}

int CRawDataProcess::InitData(CNode* pstNode, CRawData* pstAffectedRows, const char* ptrKey)
{
	int iRet;

	iRet = m_stRawData.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), ptrKey, 0);
	if(iRet != 0){
		log_error("raw-data init error: %d,%s", iRet, m_stRawData.GetErrMsg());
		return(-1);
	}
	pstNode->VDHandle() = m_stRawData.GetHandle();

	if(pstAffectedRows != NULL){
		iRet = pstAffectedRows->Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), ptrKey, 0);
		if(iRet != 0){
			log_error("raw-data init error: %d,%s", iRet, pstAffectedRows->GetErrMsg());
			return(-2);
		}
	}

	return(0);
}

int CRawDataProcess::AttachData(CNode* pstNode, CRawData* pstAffectedRows)
{
	int iRet;

	iRet = m_stRawData.Attach(pstNode->VDHandle(), m_pstTab->KeyFields()-1, m_pstTab->KeyFormat());
	if(iRet != 0){
		log_error("raw-data attach[handle:"UINT64FMT"] error: %d,%s", pstNode->VDHandle(), iRet, m_stRawData.GetErrMsg());
		return(-1);
	}

	if(pstAffectedRows != NULL){
		iRet = pstAffectedRows->Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), m_stRawData.Key(), 0);
		if(iRet != 0){
			log_error("raw-data init error: %d,%s", iRet, pstAffectedRows->GetErrMsg());
			return(-2);
		}
	}

	return(0);
}

int CRawDataProcess::GetAllRows(CNode* pstNode, CRawData* pstRows)
{
	int iRet;

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	iRet = AttachData(pstNode, pstRows);
	if(iRet != 0){
		log_error("attach data error: %d", iRet);
		return(-1);
	}

	pstRows->SetRefrence(&m_stRawData);
	if(pstRows->CopyAll() != 0){
		log_error("copy data error: %d,%s", iRet, pstRows->GetErrMsg());
		return(-2);
	}

	return(0);
}

int CRawDataProcess::DestroyData(CNode* pstNode)
{
	int iRet;

	iRet = m_stRawData.Attach(pstNode->VDHandle(), m_pstTab->KeyFields()-1, m_pstTab->KeyFormat());
	if(iRet != 0){
		log_error("raw-data attach error: %d,%s", iRet, m_stRawData.GetErrMsg());
		return(-1);
	}
	m_llRowsInc += 0LL - m_stRawData.TotalRows();

	m_stRawData.Destroy();
	pstNode->VDHandle() = INVALID_HANDLE;

	return(0);
}

int CRawDataProcess::ReplaceData(CNode* pstNode, CRawData* pstRawData)
{
	int iRet;

	log_debug("ReplaceData start ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	CRawData tmpRawData(m_pMallocator);

	iRet = tmpRawData.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), pstRawData->Key(), pstRawData->DataSize()-pstRawData->DataStart());
	if(iRet == EC_NO_MEM){
		if(m_pstPool->TryPurgeSize(tmpRawData.NeedSize(), *pstNode) == 0)
			iRet = tmpRawData.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), pstRawData->Key(), pstRawData->DataSize()-pstRawData->DataStart());
	}

	if(iRet != 0){
		snprintf(m_szErr, sizeof(m_szErr), "raw-data init error: %s", tmpRawData.GetErrMsg());
		tmpRawData.Destroy();
		return(-2);
	}

	tmpRawData.SetRefrence(pstRawData);
	iRet = tmpRawData.CopyAll();
	if(iRet != 0){
		snprintf(m_szErr, sizeof(m_szErr), "raw-data init error: %s", tmpRawData.GetErrMsg());
		tmpRawData.Destroy();
		return(-3);
	}

	if(pstNode->VDHandle() != INVALID_HANDLE)
		DestroyData(pstNode);
	pstNode->VDHandle() = tmpRawData.GetHandle();
	m_llRowsInc += pstRawData->TotalRows();
	if (tmpRawData.TotalRows() > 0)
	{
		log_debug("ReplaceData,  stat history datasize, size is %u", tmpRawData.DataSize());
		history_datasize.push(tmpRawData.DataSize());
		history_rowsize.push(tmpRawData.TotalRows());
		m_stRawData.UpdateLastAccessTimeByHour();
		m_stRawData.UpdateLastUpdateTimeByHour();
	}
	return(0);
}

