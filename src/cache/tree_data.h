#ifndef TREE_DATA_H
#define TREE_DATA_H

/************************************************************
  Description:    ��װ��t-tree���ݽṹ�洢����      
  Version:         TTC 3.0
  Function List:   
    1.  Init()			��ʼ������
   2.  InsertRow()		����һ������
   3. Find() 			����һ������
   4. Search()		����������������������
   5. TraverseForward()	��С���������������
   6. Delete()		ɾ����������������
   7. UpdateIndex()	��������ֵ
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include "raw_data.h"
#include "t_tree.h"

typedef enum _CTCheckResult{
	T_CHK_CONTINUE, // ���������������
	T_CHK_SKIP, // ��������������������������ڵ�
	T_CHK_STOP, // ��ֹ����ѭ��
	T_CHK_DESTROY // �����������
}CTCheckResult;

typedef CTCheckResult (*CheckTreeFunc)(CMallocator& stMalloc, uint8_t uchIndexCnt, uint8_t uchCurIdxCnt, const CRowValue* pstIndexValue, const uint32_t uiTreeRowNum, void* pCookie);
typedef int (*VisitRawData)(CMallocator& stMalloc, uint8_t uchIndexCnt, const CRowValue* pstIndexValue, ALLOC_HANDLE_T& hHandle, int64_t& llRowNumInc, void* pCookie);

/************************************************************
  Description:    t-tree���ڵ�����ݽṹ
  Version:         TTC 3.0
***********************************************************/
struct _CRootData{
	unsigned char	m_uchDataType;
	uint32_t m_uiRowCnt;
	MEM_HANDLE_T m_hRoot;
	char	m_achKey[0];
}__attribute__((packed));
typedef struct _CRootData CRootData;

class CTableDefinition;
typedef struct _CCmpCookie{
	const CTableDefinition* m_pstTab;
	uint8_t	m_uchIdx;
	_CCmpCookie(const CTableDefinition* pstTab, uint8_t uchIdx)
	{
		m_pstTab = pstTab;
		m_uchIdx = uchIdx;
	}
}CCmpCookie;


typedef enum _CTCondType{
	T_COND_VAL_SET, // ��ѯ�ض���ֵ�б�
	T_COND_RANGE, // ��ѯvalue[0] ~ Key-value[0]<=value[1].s64
	T_COND_GE, // ��ѯ���ڵ���value[0]��key
	T_COND_LE, // ��ѯС�ڵ���value[0]��key
	T_COND_ALL // ��������key
}CTCondType;

typedef enum _CTOrder{
	T_ORDER_ASC, // ����
	T_ORDER_DEC, // ����
	T_ORDER_POS, // �������
}CTOrder;

/************************************************************
  Description:    �������ݵ�����
  Version:         TTC 3.0
***********************************************************/
typedef struct{
	unsigned char m_uchCondType;
	unsigned char m_uchOrder;
	unsigned int m_uiValNum;
	CValue* m_pstValue;
}CTtreeCondition;


class CTreeData
{
private:
	CRootData* m_pstRootData; // ע�⣺��ַ���ܻ���Ϊrealloc���ı�
	CTtree	m_stTree;
	const CTableDefinition* m_pstTab;
	uint8_t m_uchIndexDepth;
	char	m_szErr[100];
	
	ALLOC_SIZE_T	m_uiNeedSize; // ���һ�η����ڴ�ʧ����Ҫ�Ĵ�С
	
	MEM_HANDLE_T	_handle;
    uint64_t    	_size;
    CMallocator*		_mallocator;
	
	/************************************************************
	  Description:    �ݹ�������ݵ�cookie����
	  Version:         TTC 3.0
	***********************************************************/
	typedef struct{
		CTreeData* m_pstTree;
		uint8_t	m_uchCondIdxCnt;
		uint8_t m_uchCurIndex;
		MEM_HANDLE_T m_hHandle;
		int64_t	m_llAffectRows;
		const int* piInclusion;
		KeyComparator m_pfComp;
		const CRowValue* m_pstCond;
		CRowValue* m_pstIndexValue;
		VisitRawData	m_pfVisit;
		void*		m_pCookie;
	}CIndexCookie;

	typedef struct{
		CTreeData* m_pstTree;
		uint8_t m_uchCurCond;
		MEM_HANDLE_T m_hHandle;
		int64_t	m_llAffectRows;
		const CTtreeCondition* m_pstCond;
		KeyComparator m_pfComp;
		CRowValue* m_pstIndexValue;
		CheckTreeFunc	m_pfCheck;
		VisitRawData	m_pfVisit;
		void*		m_pCookie;
	}CSearchCookie;

protected:
	template <class T>
	T* Pointer(void) const {return reinterpret_cast<T *>(_mallocator->Handle2Ptr(_handle));}
	
	inline int PackKey(const CRowValue& stRow, uint8_t uchKeyIdx, int& iKeySize, char*& pchKey, unsigned char achKeyBuf[]);
	inline int PackKey(const CValue* pstVal, uint8_t uchKeyIdx, int& iKeySize, char*& pchKey, unsigned char achKeyBuf[]);
	inline int UnpackKey(char* pchKey, uint8_t uchKeyIdx, CRowValue& stRow);
	
	int InsertSubTree(uint8_t uchCurIndex, uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T hRoot);
	int InsertSubTree(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T hRoot);
	int InsertRowFlag(uint8_t uchCurIndex, const CRowValue& stRow, KeyComparator pfComp, unsigned char uchFlag);
	int Find(CIndexCookie* pstIdxCookie);
	int Find(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	static int SearchVisit(CMallocator& stMalloc, ALLOC_HANDLE_T& hRecord, void* pCookie);
	int Search(CSearchCookie* pstSearchCookie);
	int Delete(CIndexCookie* pstIdxCookie);
	int Delete(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	
public:
	CTreeData(CMallocator* pstMalloc, const CTableDefinition* pstTab);
	virtual ~CTreeData();
	
	const char* GetErrMsg(){ return m_szErr; }
	MEM_HANDLE_T GetHandle(){ return _handle; }
	int Attach(MEM_HANDLE_T hHandle);
	
	const MEM_HANDLE_T GetTreeRoot() const { return m_stTree.Root(); }
	
	/*************************************************
	  Description:	�·���һ���ڴ棬����ʼ��
	  Input:		 iKeySize	key�ĸ�ʽ��0Ϊ�䳤����0Ϊ��������
				pchKey	Ϊ��ʽ�����key���䳤key�ĵ�0�ֽ�Ϊ����
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Init(int iKeySize, const char* pchKey);
	
	const char* Key() const { return m_pstRootData?m_pstRootData->m_achKey:NULL; }
	char* Key(){ return m_pstRootData?m_pstRootData->m_achKey:NULL; }
	
	unsigned int  TotalRows(){ return m_pstRootData->m_uiRowCnt; }
	
	/*************************************************
	  Description:	���һ�η����ڴ�ʧ������Ҫ���ڴ��С
	  Input:		
	  Output:		
	  Return:		��������Ҫ���ڴ��С
	*************************************************/
	ALLOC_SIZE_T NeedSize() { return m_uiNeedSize; }
	
	/*************************************************
	  Description:	����uchLevel�Լ����¼��������
	  Input:		uchLevel	����uchLevel�Լ����¼������������ȻuchLevelӦ����1��uchIndexDepth֮��
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
//	int Destroy(uint8_t uchLevel=1);
	int Destroy();
	
	/*************************************************
	  Description:	����һ������
	  Input:		stRow	����index�ֶ��Լ������ֶε�ֵ
				pfComp	�û��Զ����key�ȽϺ���
				uchFlag	�б��
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int InsertRowFlag(const CRowValue& stRow, KeyComparator pfComp, unsigned char uchFlag);
	
	/*************************************************
	  Description:	����һ������
	  Input:		stRow	����index�ֶ��Լ������ֶε�ֵ
				pfComp	�û��Զ����key�ȽϺ���
				isDirty	�Ƿ�������
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int InsertRow(const CRowValue& stRow, KeyComparator pfComp, bool isDirty);
	
	/*************************************************
	  Description:	����һ������
	  Input:		stCondition	��������index�ֶε�ֵ
				pfComp	�û��Զ����key�ȽϺ���
				
	  Output:		hRecord	���ҵ���һ��ָ��CRawData�ľ��
	  Return:		0Ϊ�Ҳ�����1Ϊ�ҵ�����
	*************************************************/
	int Find(const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	
	/*************************************************
	  Description:	��������������
	  Input:		pstCond	һ�����飬���Ҵ�С�պ���uchIndexDepth
				pfComp	�û��Զ����key�ȽϺ���
				pfVisit	�����ҵ���¼ʱ���û��Զ���ķ������ݺ���
				pCookie	�������ݺ���ʹ�õ�cookie����
	  Output:		
	  Return:		0Ϊ�ɹ�������ֵΪ����
	*************************************************/
	int Search(const CTtreeCondition* pstCond, KeyComparator pfComp, VisitRawData pfVisit, CheckTreeFunc pfCheck, void* pCookie);
	
	/*************************************************
	  Description:	��С���������������
	  Input:		pfComp	�û��Զ����key�ȽϺ���
				pfVisit	�����ҵ���¼ʱ���û��Զ���ķ������ݺ���
				pCookie	�������ݺ���ʹ�õ�cookie����
	  Output:		
	  Return:		0Ϊ�ɹ�������ֵΪ����
	*************************************************/
	int TraverseForward(KeyComparator pfComp, VisitRawData pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	����ָ����indexֵ��ɾ�����������������У�����������
	  Input:		uchCondIdxCnt	����index������
				stCondition		��������index�ֶε�ֵ
				pfComp		�û��Զ����key�ȽϺ���
				
	  Output:		
	  Return:		0Ϊ�ɹ�������ֵΪ����
	*************************************************/
	int Delete(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp);
	
	/*************************************************
	  Description:	��ĳ�������indexֵ�޸�Ϊ����һ��ֵ
	  Input:		uchCondIdxCnt	����index������
				stCondition		��������index�ֶε�ֵ
				pfComp		�û��Զ����key�ȽϺ���
				pstNewValue	��Ӧ���һ�������ֶε���indexֵ
	  Output:		
	  Return:		0Ϊ�ɹ�������ֵΪ����
	*************************************************/
	int UpdateIndex(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, const CValue* pstNewValue);
	unsigned AskForDestroySize(void);
};

#endif
