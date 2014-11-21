#ifndef RAW_DATA_PROCESS_H
#define RAW_DATA_PROCESS_H

/************************************************************
  Description:   ��װ��ƽ�����ݴ���ĸ��ֽӿ�
  Version:         TTC 3.0
  Function List:   
    1.  GetAllRows()	��ѯ������
   2.  DeleteData()	����taskɾ������
   3. GetData() 		��ѯ����
   4. AppendData()	����һ������
   5. ReplaceData()	��task�������滻cache�������
   6. UpdateData()	��������
   7. FlushData()		��������flush��db
   8.PurgeData()		��cache�������ɾ��
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
	
	/*����ʷ�ڵ����ݵĲ���ͳ�ƣ����ڸ߶��ڴ��������ĵط�����������ͳ�Ƶ� , modify by tomchen 2014.08.27*/
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
	  Description:	��ѯ���β������ӵ�����������Ϊ������
	  Input:		
	  Output:		
	  Return:		����
	*************************************************/
	int64_t RowsInc(){ return m_llRowsInc; }
	
	/*************************************************
	  Description:	��ѯ���β������ӵ�������������Ϊ������
	  Input:		
	  Output:		
	  Return:		����
	*************************************************/
	int64_t DirtyRowsInc(){ return m_llDirtyRowsInc; }
	
	/*************************************************
	  Description:	��ѯnode�����������
	  Input:		pstNode	node�ڵ�
	  Output:		pstRows	�������ݵĽṹ
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int GetAllRows(CNode* pstNode, CRawData* pstRows);
	
	/*************************************************
	  Description:	��pstRows�������滻cache�������
	  Input:		pstRows	������
				pstNode	node�ڵ�
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int ReplaceData(CNode* pstNode, CRawData* pstRawData);

	/*************************************************
	  Description:	����task����ɾ������
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		pstAffectedRows	���汻ɾ�������ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int DeleteData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows);
	
	/*************************************************
	  Description:	����task�����ѯ����
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		stTask	������ҵ�������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int GetData(CTaskRequest &stTask, CNode* pstNode);

	/*************************************************
	  Description:	����task�������һ������
	  Input:		stTask	task����
				pstNode	node�ڵ�
				isDirty	�Ƿ�������
	  Output:		pstAffectedRows	���汻ɾ�������ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int AppendData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool isDirty, bool uniq);
	
	/*************************************************
	  Description:	��task�������滻cache�������
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int ReplaceData(CTaskRequest &stTask, CNode* pstNode);

	/*************************************************
	  Description:	��task�������滻cache�������
	  Input:		stTask	task����
				pstNode	node�ڵ�
				async	�Ƿ��첽����
	  Output:		pstAffectedRows	���汻���º�����ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int ReplaceRows(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false);
	
	/*************************************************
	  Description:	����task�������cache����
	  Input:		stTask	task����
				pstNode	node�ڵ�
				async	�Ƿ��첽����
	  Output:		pstAffectedRows	���汻���º�����ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int UpdateData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false);
	
	/*************************************************
	  Description:	��node�ڵ��������������ɸ�flush����
	  Input:		pstNode	node�ڵ�
	  Output:		pstFlushReq	����flush����
				uiFlushRowsCnt	��flush������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int FlushData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt);
	
	/*************************************************
	  Description:	ɾ��cache������ݣ�����������ݻ�����flush����
	  Input:		pstNode	node�ڵ�
	  Output:		pstFlushReq	����flush����
				uiFlushRowsCnt	��flush������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int PurgeData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt);
};

TTC_END_NAMESPACE

#endif
