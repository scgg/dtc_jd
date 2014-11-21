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
#include "task_pkey.h"
#include "cache_flush.h"
#include "RelativeHourCalculator.h"
TTC_USING_NAMESPACE

int CRawDataProcess::DirtyRowsInNode(CTaskRequest &stTask, CNode * pstNode)
{
    int iRet = 0;
    int dirty_rows = 0;

    iRet = AttachData(pstNode, NULL);
    if(iRet != 0){
	log_error("attach data error: %d", iRet);
	return iRet;
    }

    unsigned char uchRowFlags;
    unsigned int uiTotalRows = m_stRawData.TotalRows();

    CRowValue stRow(stTask.TableDefinition());   
    for(unsigned int i=0; i<uiTotalRows; i++){
	iRet = m_stRawData.DecodeRow(stRow, uchRowFlags, 0);
	if(iRet != 0){
	    log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
	    return(-4);
	}

	if(uchRowFlags & OPER_DIRTY)
	    dirty_rows ++;
    }

    return dirty_rows;
}

int CRawDataProcess::DeleteData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows)
{
	int iRet;

	log_debug("DeleteData start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	iRet = AttachData(pstNode, pstAffectedRows);
	if(iRet != 0){
		log_error("attach data error: %d", iRet);
		return(iRet);
	}

	unsigned char uchRowFlags;
	unsigned int uiTotalRows = m_stRawData.TotalRows();
	if(pstAffectedRows != NULL)
		pstAffectedRows->SetRefrence(&m_stRawData);

	int iAffectRows = 0;
	CRowValue stRow(stTask.TableDefinition()); 		//一行数据
	for(unsigned int i=0; i<uiTotalRows; i++){
		iRet = m_stRawData.DecodeRow(stRow, uchRowFlags, 0);
		if(iRet != 0){
			log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
			return(-4);
		}
		if(stTask.CompareRow(stRow) != 0){ //符合del条件	
			if(pstAffectedRows != NULL){ // copy row 
				iRet = pstAffectedRows->CopyRow();
				if(iRet != 0){
					log_error("raw-data copy row error: %d,%s", iRet, pstAffectedRows->GetErrMsg());
				}
			}
			iRet = m_stRawData.DeleteCurRow(stRow); 
			if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();
			if(iRet != 0){
				log_error("raw-data delete row error: %d,%s", iRet, m_stRawData.GetErrMsg());
				return(-5);
			}
			iAffectRows++;
			m_llRowsInc--;
			if(uchRowFlags & OPER_DIRTY)
				m_llDirtyRowsInc--;
		}
	}
	if(iAffectRows > 0) {
		if(stTask.resultInfo.AffectedRows()==0 ||
				(stTask.RequestCondition()&&stTask.RequestCondition()->HasTypeTimestamp()))
		{
			stTask.resultInfo.SetAffectedRows(iAffectRows);
		}
		m_stRawData.StripMem();
	}

	
	if (m_stRawData.TotalRows() > 0)
	{
		log_debug("stat history datasize, size is %u", m_stRawData.DataSize());
		history_datasize.push(m_stRawData.DataSize());
		history_rowsize.push(m_stRawData.TotalRows());
		m_stRawData.UpdateLastAccessTimeByHour();
		m_stRawData.UpdateLastUpdateTimeByHour();
	}
	return(0);
}

int CRawDataProcess::GetData(CTaskRequest &stTask, CNode* pstNode)
{
	int iRet;

	log_debug("GetData start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;
	int laid = stTask.FlagNoCache() ? -1 : stTask.TableDefinition()->LastaccFieldId();

	iRet = m_stRawData.Attach(pstNode->VDHandle(), m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), laid);
	if(iRet != 0){
		log_error("raw-data attach[handle:"UINT64FMT"] error: %d,%s", pstNode->VDHandle(), iRet, m_stRawData.GetErrMsg());
		return(-1);
	}

	unsigned char uchRowFlags;
	unsigned int uiTotalRows = m_stRawData.TotalRows();
	stTask.PrepareResult();				//准备返回结果对象
	if(stTask.AllRows() && (stTask.CountOnly() || !stTask.InRange((int)uiTotalRows, 0))){
		if(stTask.IsBatchRequest()) {
			if((int)uiTotalRows > 0)
				stTask.AddTotalRows((int)uiTotalRows);
		} else {
			stTask.SetTotalRows((int)uiTotalRows); 
		}
	}
	else{
		CRowValue stRow(stTask.TableDefinition()); 			//一行数据
		if(stTask.TableDefinition()->HasDiscard())
			stRow.DefaultValue();
		for(unsigned int i = 0; i <uiTotalRows; i++ )	//逐行拷贝数据
		{
			stTask.UpdateKey(stRow);
			if((iRet = m_stRawData.DecodeRow(stRow, uchRowFlags, 0)) != 0){
				log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
				return(-2);
			}
			if(stTask.CompareRow(stRow) == 0)//如果不符合查询条件
				continue;

			//当前行添加到task中
			if(stTask.AppendRow(&stRow) > 0 && laid > 0) {
				m_stRawData.UpdateLastacc(stTask.Timestamp());
			}
			if(stTask.AllRows() && stTask.ResultFull()) {
				stTask.SetTotalRows((int)uiTotalRows);
				break;
			}
		}
	}
	/*更新访问时间和查找操作计数*/
	m_stRawData.UpdateLastAccessTimeByHour();	
	m_stRawData.IncSelectCount();
	log_debug("node[id:%u] ，Get Count is %d, LastAccessTime is %d, CreateTime is %d", 	pstNode->NodeID(), 
			m_stRawData.GetSelectOpCount(),	m_stRawData.GetLastAccessTimeByHour(), m_stRawData.GetCreateTimeByHour());
	return(0);
}

int CRawDataProcess::AppendData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool isDirty, bool setrows)
{
	int iRet;
	CRowValue stRow(stTask.TableDefinition());
	stRow.DefaultValue();
	stTask.UpdateRow(stRow);//获取新插入的行

	log_debug("AppendData start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	if(m_pstTab->AutoIncrementFieldId()>=m_pstTab->KeyFields() && stTask.resultInfo.InsertID()){
		const int iFieldID = m_pstTab->AutoIncrementFieldId();
		const uint64_t iVal = stTask.resultInfo.InsertID();
		stRow.FieldValue(iFieldID)->Set(iVal);
	}

	iRet = AttachData(pstNode, pstAffectedRows);
	if(iRet != 0){
		snprintf(m_szErr, sizeof(m_szErr), "attach data error");
		log_warning("attach data error: %d", iRet);
		return(iRet);
	}

	unsigned int uiTotalRows = m_stRawData.TotalRows();
	if(uiTotalRows > 0) {
		if((isDirty || setrows ) && stTask.TableDefinition()->KeyAsUniqField()) {
			snprintf(m_szErr, sizeof(m_szErr), "duplicate key error");
			return(-1062);
		}
		CRowValue stOldRow(stTask.TableDefinition()); 			//一行数据
		if(setrows && stTask.TableDefinition()->KeyPartOfUniqField()){
			for(unsigned int i = 0; i <uiTotalRows; i++ ){				//逐行拷贝数据
				unsigned char uchRowFlags;
				if(m_stRawData.DecodeRow(stOldRow, uchRowFlags, 0) != 0){
					log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
					return(-1);
				}

				if(stRow.Compare(stOldRow, stTask.TableDefinition()->UniqFieldsList(), 
							stTask.TableDefinition()->UniqFields()) == 0)
				{
					snprintf(m_szErr, sizeof(m_szErr), "duplicate key error");
					return(-1062);
				}
			}
		}
	}

	if(pstAffectedRows!=NULL && pstAffectedRows->InsertRow(stRow, false, isDirty) != 0){
		snprintf(m_szErr, sizeof(m_szErr), "raw-data insert row error: %s", pstAffectedRows->GetErrMsg());
		return(-1);
	}

	// insert clean row
	iRet = m_stRawData.InsertRow(stRow, m_stUpdateMode.m_uchInsertOrder?true:false, isDirty);
	if(iRet == EC_NO_MEM){
		if(m_pstPool->TryPurgeSize(m_stRawData.NeedSize(), *pstNode) == 0)
			iRet = m_stRawData.InsertRow(stRow, m_stUpdateMode.m_uchInsertOrder?true:false, isDirty);
	}
	if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();
	if(iRet != 0)
	{
		snprintf(m_szErr, sizeof(m_szErr), "raw-data insert row error: %s", m_stRawData.GetErrMsg());
		/*标记加入黑名单*/
		stTask.PushBlackListSize(m_stRawData.NeedSize());
		return(-2);
	}

	if(stTask.resultInfo.AffectedRows()==0 || setrows==true)
		stTask.resultInfo.SetAffectedRows(1);
	m_llRowsInc++;
	if(isDirty)
		m_llDirtyRowsInc++;
	log_debug("stat history datasize, size is %u", m_stRawData.DataSize());	
	history_datasize.push(m_stRawData.DataSize());
	history_rowsize.push(m_stRawData.TotalRows());
	m_stRawData.UpdateLastAccessTimeByHour();
	m_stRawData.UpdateLastUpdateTimeByHour();
			log_debug("node[id:%u] ，Get Count is %d, CreateTime is %d, LastAccessTime is %d, LastUpdateTime is %d ", pstNode->NodeID(), 
			m_stRawData.GetSelectOpCount(),	m_stRawData.GetCreateTimeByHour(),
			m_stRawData.GetLastAccessTimeByHour(), m_stRawData.GetLastUpdateTimeByHour());
	return(0);	
}

int CRawDataProcess::ReplaceData(CTaskRequest &stTask, CNode* pstNode)
{
	log_debug("ReplaceData start! ");

	int iRet;
	int try_purge_count = 0;
	uint64_t all_rows_size = 0;
	int laid = stTask.FlagNoCache() || stTask.CountOnly() ? -1 : stTask.TableDefinition()->LastaccFieldId();
	int matchedCount = 0;
	int limitStart = 0;
	int limitStop = 0x10000000;
	if(laid > 0 && stTask.requestInfo.LimitCount() > 0) {
		limitStart = stTask.requestInfo.LimitStart();
		if(stTask.requestInfo.LimitStart() > 0x10000000) {
			laid = -1;
		} else if(stTask.requestInfo.LimitCount() < 0x10000000) {
			limitStop = limitStart + stTask.requestInfo.LimitCount();
		}
	}

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	if(pstNode->VDHandle() != INVALID_HANDLE){
		iRet = DestroyData(pstNode);
		if(iRet != 0)
			return(-1);
	}

	iRet = m_stRawData.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), stTask.PackedKey(), 0);
	if(iRet == EC_NO_MEM){
		if(m_pstPool->TryPurgeSize(m_stRawData.NeedSize(), *pstNode) == 0)
			iRet = m_stRawData.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), stTask.PackedKey(), 0);
	}
	if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();

	if(iRet != 0){
		snprintf(m_szErr, sizeof(m_szErr), "raw-data init error: %s", m_stRawData.GetErrMsg());
		/*标记加入黑名单*/
		stTask.PushBlackListSize(m_stRawData.NeedSize());
		m_pstPool->PurgeNode(stTask.PackedKey(), *pstNode);
		return(-2);
	}

	if(stTask.result != NULL){
		CResultSet* pstResultSet = stTask.result;
		for(int i=0; i<pstResultSet->TotalRows() ; i++)
		{
			CRowValue* pstRow = pstResultSet->__FetchRow();
			if(pstRow == NULL){
				log_debug("%s!","call FetchRow func error");
				m_pstPool->PurgeNode(stTask.PackedKey(), *pstNode);
				m_stRawData.Destroy();
				return(-3);
			}

			if(laid > 0 && stTask.CompareRow(*pstRow)) {
				if(matchedCount >= limitStart && matchedCount < limitStop) {
					(*pstRow)[laid].s64 = stTask.Timestamp();
				}
				matchedCount++;
			}

			/* 插入当前行 */
			iRet = m_stRawData.InsertRow(*pstRow, false, false);

			/* 如果内存空间不足，尝试扩大最多两次 */
			if(iRet == EC_NO_MEM){
				
				/* 预测整个Node的数据大小 */
				all_rows_size = m_stRawData.NeedSize() - m_stRawData.DataStart();
				all_rows_size *= pstResultSet->TotalRows();
				all_rows_size /= (i+1);
				all_rows_size += m_stRawData.DataStart();

				if(try_purge_count >= 2 ){
					goto ERROR_PROCESS;
				}

				/* 尝试次数 */
				++try_purge_count;
				if(m_pstPool->TryPurgeSize((size_t)all_rows_size, *pstNode) == 0)
					iRet = m_stRawData.InsertRow(*pstRow, false, false);

			}
			if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();

			/* 当前行操作成功 */
			if(0 == iRet)
				continue;
ERROR_PROCESS:
			snprintf(m_szErr, sizeof(m_szErr),"raw-data insert row error: ret=%d,err=%s, cnt=%d", 
					iRet, m_stRawData.GetErrMsg(),try_purge_count);
			/*标记加入黑名单*/
			stTask.PushBlackListSize(all_rows_size);
			m_pstPool->PurgeNode(stTask.PackedKey(), *pstNode);
			m_stRawData.Destroy();
			return(-4);
		}

		m_llRowsInc += pstResultSet->TotalRows();
	}

	m_stRawData.UpdateLastAccessTimeByHour();
	m_stRawData.UpdateLastUpdateTimeByHour();
	log_debug("node[id:%u], handle["UINT64FMT"] ,data-size[%u],  Get Count is %d, CreateTime is %d, LastAccessTime is %d, Update time is %d", 
			pstNode->NodeID(), 
			pstNode->VDHandle(),
			m_stRawData.DataSize(),
			m_stRawData.GetSelectOpCount(),
			m_stRawData.GetCreateTimeByHour(),
			m_stRawData.GetLastAccessTimeByHour(),
			m_stRawData.GetLastUpdateTimeByHour());

	
	history_datasize.push(m_stRawData.DataSize());
	history_rowsize.push(m_stRawData.TotalRows());
	return(0);
}

// The correct replace behavior:
// 	If conflict rows found, delete them all
// 	Insert new row
// 	Affected rows is total deleted and inserted rows
// Implementation hehavior:
// 	If first conflict row found, update it, and increase affected rows to 2 (1 delete + 1 insert)
// 	delete other fonflict row, increase affected 1 per row
// 	If no rows found, insert it and set affected rows to 1
int CRawDataProcess::ReplaceRows(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows)
{
	int iRet;

	log_debug("ReplaceRows start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	if(pstNode->VDHandle() == INVALID_HANDLE){
		iRet = InitData(pstNode, pstAffectedRows, stTask.PackedKey());
		if(iRet != 0){
			log_error("init data error: %d", iRet);
			if(pstNode->VDHandle() == INVALID_HANDLE)
				m_pstPool->PurgeNode(stTask.PackedKey(), *pstNode);
			return(iRet);
		}
	}
	else{
		iRet = AttachData(pstNode, pstAffectedRows);
		if(iRet != 0){
			log_error("attach data error: %d", iRet);
			return(iRet);
		}
	}

	unsigned char uchRowFlags;
	uint64_t ullAffectedrows = 0;
	unsigned int uiTotalRows = m_stRawData.TotalRows();
	if(pstAffectedRows != NULL)
		pstAffectedRows->SetRefrence(&m_stRawData);

	CRowValue stNewRow(stTask.TableDefinition());
	stNewRow.DefaultValue();
	stTask.UpdateRow(stNewRow);			//获取Replace的行

	CRowValue stRow(stTask.TableDefinition()); 			//一行数据
	for(unsigned int i = 0; i <uiTotalRows; i++ ){				//逐行拷贝数据
		if(m_stRawData.DecodeRow(stRow, uchRowFlags, 0) != 0){
			log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
			return(-1);
		}

		if(stTask.TableDefinition()->KeyAsUniqField() == false && 
				stNewRow.Compare(stRow, stTask.TableDefinition()->UniqFieldsList(), 
					stTask.TableDefinition()->UniqFields()) != 0)
			continue;

		if(ullAffectedrows==0) {
			if(pstAffectedRows != NULL && pstAffectedRows->InsertRow(stNewRow, false, async) != 0){
				log_error("raw-data copy row error: %d,%s", iRet, pstAffectedRows->GetErrMsg());
				return(-2);
			}

			ullAffectedrows = 2;
			iRet = m_stRawData.ReplaceCurRow(stNewRow, async); // 加进cache
		} else {
			ullAffectedrows++;
			iRet = m_stRawData.DeleteCurRow(stNewRow); // 加进cache
		}
		if(iRet == EC_NO_MEM){
			if(m_pstPool->TryPurgeSize(m_stRawData.NeedSize(), *pstNode) == 0)
				iRet =m_stRawData.ReplaceCurRow(stNewRow, async);
		}
		if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();
		if(iRet != 0){
			snprintf(m_szErr, sizeof(m_szErr), "raw-data replace row error: %d, %s",
					iRet, m_stRawData.GetErrMsg());
			/*标记加入黑名单*/
			stTask.PushBlackListSize(m_stRawData.NeedSize());
			return(-3);
		}
		if(uchRowFlags & OPER_DIRTY)
			m_llDirtyRowsInc--;
		if(async)
			m_llDirtyRowsInc++;

		//This is not true now
		//break; // 只能有一行被修改
	}

	if(ullAffectedrows == 0){ 	// 找不到匹配的行，insert一行
		iRet = m_stRawData.InsertRow(stNewRow, false, async); // 加进cache
		if(iRet == EC_NO_MEM){
			if(m_pstPool->TryPurgeSize(m_stRawData.NeedSize(), *pstNode) == 0)
				iRet =m_stRawData.InsertRow(stNewRow, false, async);
		}
		if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();

		if(iRet != 0){
			snprintf(m_szErr, sizeof(m_szErr), "raw-data replace row error: %d, %s", 
					iRet, m_stRawData.GetErrMsg());
			/*标记加入黑名单*/
			stTask.PushBlackListSize(m_stRawData.NeedSize());
			return(-3);
		}
		m_llRowsInc++;
		ullAffectedrows++;
		if(async)
			m_llDirtyRowsInc++;
	}

	if (async == true || setrows==true) {
		stTask.resultInfo.SetAffectedRows(ullAffectedrows);
	}
	else if(ullAffectedrows != stTask.resultInfo.AffectedRows()){
		//如果cache更新纪录数和helper更新的纪录数不相等
		log_debug("unequal affected rows, cache[%lld], helper[%lld]", 
				(long long)ullAffectedrows,
				(long long)stTask.resultInfo.AffectedRows());        
	}	
	
	log_debug("stat history datasize, size is %u", m_stRawData.DataSize());
	history_datasize.push(m_stRawData.DataSize());
	history_rowsize.push(m_stRawData.TotalRows());
	m_stRawData.UpdateLastAccessTimeByHour();
	m_stRawData.UpdateLastUpdateTimeByHour();
		log_debug("node[id:%u], CreateTime is %d, LastAccessTime is %d, Update Time is %d ", 
				pstNode->NodeID(),m_stRawData.GetCreateTimeByHour(),m_stRawData.GetLastAccessTimeByHour(), m_stRawData.GetLastUpdateTimeByHour());
	return(0);
}

/*
 * encode到私有内存，防止replace，update引起重新rellocate导致value引用了过期指针
 */
int CRawDataProcess::encode_to_private_area (CRawData& raw, CRowValue& value, unsigned char value_flag)
{
	int ret = raw.Init(m_pstTab->KeyFields()-1, m_pstTab->KeyFormat(), m_stRawData.Key(), raw.CalcRowSize(value,m_pstTab->KeyFields()-1));
	if(0 != ret)
	{
		log_error("init raw-data struct error, ret=%d, err=%s", ret, raw.GetErrMsg());
		return -1;
	}

	ret = raw.InsertRow(value, false, false);
	if(0 != ret)
	{
		log_error("insert row to raw-data error: ret=%d, err=%s", ret, raw.GetErrMsg());
		return -2;
	}

	raw.Rewind();

	ret = raw.DecodeRow(value,  value_flag, 0);
	if(0 != ret)
	{
		log_error("decode raw-data to row error: ret=%d, err=%s", ret, raw.GetErrMsg());
		return -3;
	}
	
	return 0;
}

int CRawDataProcess::UpdateData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows)
{
	int iRet;

	log_debug("UpdateData start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	iRet = AttachData(pstNode, pstAffectedRows);
	if(iRet != 0){
		log_error("attach data error: %d", iRet);
		return(iRet);
	}

	unsigned char uchRowFlags;
	uint64_t ullAffectedrows = 0;
	unsigned int uiTotalRows = m_stRawData.TotalRows();
	if(pstAffectedRows != NULL)
		pstAffectedRows->SetRefrence(&m_stRawData);

	CRowValue stRow(stTask.TableDefinition()); 			//一行数据
	for(unsigned int i = 0; i <uiTotalRows; i++ ){				//逐行拷贝数据
		if(m_stRawData.DecodeRow(stRow, uchRowFlags, 0) != 0){
			log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
			return(-1);
		}
		
		//如果不符合查询条件
		if(stTask.CompareRow(stRow) == 0)
			continue;

		stTask.UpdateRow(stRow); 		//修改数据
		ullAffectedrows++;

		if(pstAffectedRows != NULL && pstAffectedRows->InsertRow(stRow, false, async) != 0){
			log_error("raw-data copy row error: %d,%s", iRet, pstAffectedRows->GetErrMsg());
			return(-2);
		}

		// 在私有区间decode 
		CRawData stTmpRows(&g_stSysMalloc, 1);
		if(encode_to_private_area(stTmpRows, stRow, uchRowFlags))
		{
			log_error("encode rowvalue to private rawdata area failed");
			return -3;
		}

		iRet = m_stRawData.ReplaceCurRow(stRow, async); // 加进cache
		if(iRet == EC_NO_MEM){
			if(m_pstPool->TryPurgeSize(m_stRawData.NeedSize(), *pstNode) == 0)
				iRet = m_stRawData.ReplaceCurRow(stRow, async);
		}
		if(iRet != EC_NO_MEM) pstNode->VDHandle() = m_stRawData.GetHandle();
		if(iRet != 0){
			snprintf(m_szErr, sizeof(m_szErr), "raw-data replace row error: %d, %s", 
					iRet, m_stRawData.GetErrMsg());
			/*标记加入黑名单*/
			stTask.PushBlackListSize(m_stRawData.NeedSize());
			return(-6);
		}

		if(uchRowFlags & OPER_DIRTY)
			m_llDirtyRowsInc--;
		if(async)
			m_llDirtyRowsInc++;
	}

	if (async == true || setrows==true) {
		stTask.resultInfo.SetAffectedRows(ullAffectedrows);
	}
	else if(ullAffectedrows != stTask.resultInfo.AffectedRows()){
		//如果cache更新纪录数和helper更新的纪录数不相等
		log_debug("unequal affected rows, cache[%lld], helper[%lld]",
				(long long)ullAffectedrows,
				(long long)stTask.resultInfo.AffectedRows());        
	}
	log_debug("stat history datasize, size is %u", m_stRawData.DataSize());	
	history_datasize.push(m_stRawData.DataSize());
	history_rowsize.push(m_stRawData.TotalRows());
	m_stRawData.UpdateLastAccessTimeByHour();
	m_stRawData.UpdateLastUpdateTimeByHour();
	log_debug("node[id:%u], CreateTime is %d, LastAccessTime is %d, UpdateTime is %d", 
				pstNode->NodeID(),m_stRawData.GetCreateTimeByHour(),m_stRawData.GetLastAccessTimeByHour(), m_stRawData.GetLastUpdateTimeByHour());
	return(0);
}

int CRawDataProcess::FlushData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)
{
	int iRet;

	log_debug("FlushData start! ");

	m_llRowsInc = 0;
	m_llDirtyRowsInc = 0;

	iRet = AttachData(pstNode, NULL);
	if(iRet != 0){
		log_error("attach data error: %d", iRet);
		return(iRet);
	}

	unsigned char uchRowFlags;
	unsigned int uiTotalRows = m_stRawData.TotalRows();

	uiFlushRowsCnt = 0;
	CValue astKey[m_pstTab->KeyFields()];
	CTaskPackedKey::UnpackKey(m_pstTab, m_stRawData.Key(), astKey);
	CRowValue stRow(m_pstTab); 			//一行数据
	for(int i=0; i<m_pstTab->KeyFields(); i++)
		stRow[i] = astKey[i];

	for(unsigned int i = 0; pstNode->IsDirty() && i < uiTotalRows; i++ ){	//逐行拷贝数据
		if(m_stRawData.DecodeRow(stRow, uchRowFlags, 0) != 0){
			log_error("raw-data decode row error: %d,%s", iRet, m_stRawData.GetErrMsg());
			return(-1);
		}

		if((uchRowFlags & OPER_DIRTY) == false)
			continue;

		if(pstFlushReq && pstFlushReq->FlushRow(stRow) != 0){
			log_error("FlushData() invoke flushRow() failed.");
			return(-2);
		}
		m_stRawData.SetCurRowFlag(uchRowFlags & ~OPER_DIRTY);
		m_llDirtyRowsInc--;
		uiFlushRowsCnt++;
	}

	return(0);
}

int CRawDataProcess::PurgeData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)
{
	int iRet;

	log_debug("PurgeData start! ");

	iRet = FlushData(pstFlushReq, pstNode, uiFlushRowsCnt);
	if(iRet != 0){
		return(iRet);
	}
	m_llRowsInc = 0LL - m_stRawData.TotalRows();
	//	m_pstPool->PurgeNode(stTask.PackedKey(), *pstNode);

	return(0);
}

