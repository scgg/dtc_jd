#ifndef RAW_DATA_PROCESS_H
#define RAW_DATA_PROCESS_H

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

#include "cache_def.h"
#include "protocol.h"
#include "value.h"
#include "field.h"
#include "section.h"
#include "tabledef.h"
#include "task_request.h"
#include "StatTTC.h"
#include "raw_data.h"
#include "node.h"
#include "data_process.h"
#include "cache_pool.h"
#include "namespace.h"
#include "StatMgr.h"

TTC_BEGIN_NAMESPACE

class CTaskRequest;
class CFlushRequest;

class CRawDataProcess
#if HAS_TREE_DATA
	: public CDataProcess
#endif
{
private:
	CRawData	m_stRawData;
	CTableDefinition* m_pstTab;
	CMallocator *m_pMallocator;
	CCachePool*	m_pstPool;
	CUpdateMode m_stUpdateMode;
	int64_t	m_llRowsInc;
	int64_t	m_llDirtyRowsInc;
	char m_szErr[200];
	
	unsigned int nodeSizeLimit; // -DEBUG- 
	
	/*对历史节点数据的采样统计，放在高端内存操作管理的地方，便于收敛统计点 , modify by tomchen 2014.08.27*/
	CStatSample  history_datasize;
	CStatSample  history_rowsize;
	
	
protected:
	int InitData(CNode* pstNode, CRawData* pstAffectedRows, const char* ptrKey);
	int AttachData(CNode* pstNode, CRawData* pstAffectedRows);
	int DestroyData(CNode* pstNode);

private:
	int encode_to_private_area(CRawData&, CRowValue&, unsigned char);
public:
	CRawDataProcess(CMallocator* pstMalloc, CTableDefinition* pstTab, CCachePool* pstPool, const CUpdateMode* pstUpdateMode);
#if HAS_TREE_DATA
	virtual
#endif
	~CRawDataProcess();

	void SetLimitNodeSize(int node_size) { nodeSizeLimit = node_size; } // -DEBUG-
	
	const char* GetErrMsg(){ return m_szErr; }
	void SetInsertMode(TUpdateMode iMode){ m_stUpdateMode.m_iInsertMode = iMode; }
	void SetInsertOrder(int iOrder){ m_stUpdateMode.m_uchInsertOrder = iOrder; }
	
	/*count dirty row, cache process will use it when cache_delete_rows in task->AllRows case -- newman*/
	int DirtyRowsInNode(CTaskRequest &stTask, CNode * node);

	/*************************************************
	  Description:	查询本次操作增加的行数（可以为负数）
	  Input:		
	  Output:		
	  Return:		行数
	*************************************************/
	int64_t RowsInc(){ return m_llRowsInc; }
	
	/*************************************************
	  Description:	查询本次操作增加的脏行数（可以为负数）
	  Input:		
	  Output:		
	  Return:		行数
	*************************************************/
	int64_t DirtyRowsInc(){ return m_llDirtyRowsInc; }
	
	/*************************************************
	  Description:	查询node里的所有数据
	  Input:		pstNode	node节点
	  Output:		pstRows	保存数据的结构
	  Return:		0为成功，非0失败
	*************************************************/
	int GetAllRows(CNode* pstNode, CRawData* pstRows);
	
	/*************************************************
	  Description:	用pstRows的数据替换cache里的数据
	  Input:		pstRows	新数据
				pstNode	node节点
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int ReplaceData(CNode* pstNode, CRawData* pstRawData);

	/*************************************************
	  Description:	根据task请求删除数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		pstAffectedRows	保存被删除的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	int DeleteData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows);
	
	/*************************************************
	  Description:	根据task请求查询数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		stTask	保存查找到的数据
	  Return:		0为成功，非0失败
	*************************************************/
	int GetData(CTaskRequest &stTask, CNode* pstNode);

	/*************************************************
	  Description:	根据task请求添加一行数据
	  Input:		stTask	task请求
				pstNode	node节点
				isDirty	是否脏数据
	  Output:		pstAffectedRows	保存被删除的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	int AppendData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool isDirty, bool uniq);
	
	/*************************************************
	  Description:	用task的数据替换cache里的数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int ReplaceData(CTaskRequest &stTask, CNode* pstNode);

	/*************************************************
	  Description:	用task的数据替换cache里的数据
	  Input:		stTask	task请求
				pstNode	node节点
				async	是否异步操作
	  Output:		pstAffectedRows	保存被更新后的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	int ReplaceRows(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false);
	
	/*************************************************
	  Description:	根据task请求更新cache数据
	  Input:		stTask	task请求
				pstNode	node节点
				async	是否异步操作
	  Output:		pstAffectedRows	保存被更新后的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	int UpdateData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false);
	
	/*************************************************
	  Description:	将node节点的脏数据组成若干个flush请求
	  Input:		pstNode	node节点
	  Output:		pstFlushReq	保存flush请求
				uiFlushRowsCnt	被flush的行数
	  Return:		0为成功，非0失败
	*************************************************/
	int FlushData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt);
	
	/*************************************************
	  Description:	删除cache里的数据，如果有脏数据会生成flush请求
	  Input:		pstNode	node节点
	  Output:		pstFlushReq	保存flush请求
				uiFlushRowsCnt	被flush的行数
	  Return:		0为成功，非0失败
	*************************************************/
	int PurgeData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt);
};

TTC_END_NAMESPACE

#endif
