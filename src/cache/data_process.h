#ifndef DATA_PROCESS_H
#define DATA_PROCESS_H

/************************************************************
  Description:   ���������ݴ���ĸ��ֽӿڣ��������ࣩ
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
	  Description:	��ѯ���β������ӵ�����������Ϊ������
	  Input:		
	  Output:		
	  Return:		����
	*************************************************/
	virtual int64_t RowsInc()=0;
	
	/*************************************************
	  Description:	��ѯ���β������ӵ�������������Ϊ������
	  Input:		
	  Output:		
	  Return:		����
	*************************************************/
	virtual int64_t DirtyRowsInc()=0;
	
	/*************************************************
	  Description:	��ѯnode�����������
	  Input:		pstNode	node�ڵ�
	  Output:		pstRows	�������ݵĽṹ
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int GetAllRows(CNode* pstNode, CRawData* pstRows)=0;
	
	/*************************************************
	  Description:	��pstRows�������滻cache�������
	  Input:		pstRows	������
				pstNode	node�ڵ�
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int ReplaceData(CNode* pstNode, CRawData* pstRawData)=0;

	/*************************************************
	  Description:	����task����ɾ������
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		pstAffectedRows	���汻ɾ�������ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int DeleteData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows)=0;
	
	/*************************************************
	  Description:	����task�����ѯ����
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		stTask	������ҵ�������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int GetData(CTaskRequest &stTask, CNode* pstNode)=0;

	/*************************************************
	  Description:	����task�������һ������
	  Input:		stTask	task����
				pstNode	node�ڵ�
				isDirty	�Ƿ�������
	  Output:		pstAffectedRows	���汻ɾ�������ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int AppendData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool isDirty, bool uniq)=0;
	
	/*************************************************
	  Description:	��task�������滻cache�������
	  Input:		stTask	task����
				pstNode	node�ڵ�
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int ReplaceData(CTaskRequest &stTask, CNode* pstNode)=0;

	/*************************************************
	  Description:	��task�������滻cache�������
	  Input:		stTask	task����
				pstNode	node�ڵ�
				async	�Ƿ��첽����
	  Output:		pstAffectedRows	���汻���º�����ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int ReplaceRows(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false)=0;
	
	/*************************************************
	  Description:	����task�������cache����
	  Input:		stTask	task����
				pstNode	node�ڵ�
				async	�Ƿ��첽����
	  Output:		pstAffectedRows	���汻���º�����ݣ�ΪNULLʱ�����棩
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int UpdateData(CTaskRequest &stTask, CNode* pstNode, CRawData* pstAffectedRows, bool async, bool setrows=false)=0;
	
	/*************************************************
	  Description:	��node�ڵ��������������ɸ�flush����
	  Input:		pstNode	node�ڵ�
	  Output:		pstFlushReq	����flush����
				uiFlushRowsCnt	��flush������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int FlushData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)=0;
	
	/*************************************************
	  Description:	ɾ��cache������ݣ�����������ݻ�����flush����
	  Input:		pstNode	node�ڵ�
	  Output:		pstFlushReq	����flush����
				uiFlushRowsCnt	��flush������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int PurgeData(CFlushRequest* pstFlushReq, CNode* pstNode, unsigned int& uiFlushRowsCnt)=0;
};
#else
class CRawDataProcess;
typedef class CRawDataProcess CDataProcess;
#endif

TTC_END_NAMESPACE

#endif
