#ifndef DATA_PROCESS_H
#define DATA_PROCESS_H

/************************************************************
  Description:   定义了数据处理的各种接口（纯抽象类）
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

#include "namespace.h"
TTC_BEGIN_NAMESPACE

enum TUpdateMode { MODE_SYNC=0, MODE_ASYNC, MODE_FLUSH };

typedef struct{
	TUpdateMode	m_iAsyncServer;
	TUpdateMode	m_iUpdateMode;
	TUpdateMode m_iInsertMode;
	unsigned char	m_uchInsertOrder;
}CUpdateMode;

#if HAS_TREE_DATA
class CFlushRequest;
class CDataProcess{
public:
	CDataProcess(){}
	virtual ~CDataProcess(){}
	
	virtual const char* GetErrMsg()=0;
	virtual void SetInsertMode(TUpdateMode iMode)=0;
	virtual void SetInsertOrder(int iOrder)=0;
	
	/*************************************************
	  Description:	查询本次操作增加的行数（可以为负数）
	  Input:		
	  Output:		
	  Return:		行数
	*************************************************/
	virtual int64_t RowsInc()=0;
	
	/*************************************************
	  Description:	查询本次操作增加的脏行数（可以为负数）
	  Input:		
	  Output:		
	  Return:		行数
	*************************************************/
	virtual int64_t DirtyRowsInc()=0;
	
	/*************************************************
	  Description:	查询node里的所有数据
	  Input:		pstNode	node节点
	  Output:		pstRows	保存数据的结构
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int GetAllRows(CNode* pstNode, CRawData* pstRows)=0;
	
	/*************************************************
	  Description:	用pstRows的数据替换cache里的数据
	  Input:		pstRows	新数据
				pstNode	node节点
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int ReplaceData(CNode* pstNode, CRawData* pstRawData)=0;

	/*************************************************
	  Description:	根据task请求删除数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		pstAffectedRows	保存被删除的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int DeleteData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows)=0;
	
	/*************************************************
	  Description:	根据task请求查询数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		stTask	保存查找到的数据
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int GetData(CTaskRequest &stTask, CNode* pstNode)=0;

	/*************************************************
	  Description:	根据task请求添加一行数据
	  Input:		stTask	task请求
				pstNode	node节点
				isDirty	是否脏数据
	  Output:		pstAffectedRows	保存被删除的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int AppendData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool isDirty, bool uniq)=0;
	
	/*************************************************
	  Description:	用task的数据替换cache里的数据
	  Input:		stTask	task请求
				pstNode	node节点
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int ReplaceData(CTaskRequest &stTask, CNode* pstNode)=0;

	/*************************************************
	  Description:	用task的数据替换cache里的数据
	  Input:		stTask	task请求
				pstNode	node节点
				async	是否异步操作
	  Output:		pstAffectedRows	保存被更新后的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int ReplaceRows(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false)=0;
	
	/*************************************************
	  Description:	根据task请求更新cache数据
	  Input:		stTask	task请求
				pstNode	node节点
				async	是否异步操作
	  Output:		pstAffectedRows	保存被更新后的数据（为NULL时不保存）
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int UpdateData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false)=0;
	
	/*************************************************
	  Description:	将node节点的脏数据组成若干个flush请求
	  Input:		pstNode	node节点
	  Output:		pstFlushReq	保存flush请求
				uiFlushRowsCnt	被flush的行数
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int FlushData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)=0;
	
	/*************************************************
	  Description:	删除cache里的数据，如果有脏数据会生成flush请求
	  Input:		pstNode	node节点
	  Output:		pstFlushReq	保存flush请求
				uiFlushRowsCnt	被flush的行数
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int PurgeData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)=0;
};
#else
class CRawDataProcess;
typedef class CRawDataProcess CDataProcess;
#endif

TTC_END_NAMESPACE

#endif
