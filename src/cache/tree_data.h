#ifndef TREE_DATA_H
#define TREE_DATA_H

/************************************************************
  Description:    封装了t-tree数据结构存储操作      
  Version:         TTC 3.0
  Function List:   
    1.  Init()			初始化数据
   2.  InsertRow()		插入一行数据
   3. Find() 			查找一行数据
   4. Search()		遍历符合条件的所有数据
   5. TraverseForward()	从小到大遍历所有数据
   6. Delete()		删除符合条件的数据
   7. UpdateIndex()	更新索引值
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include "raw_data.h"
#include "t_tree.h"

typedef enum _CTCheckResult{
	T_CHK_CONTINUE, // 继续访问这棵子树
	T_CHK_SKIP, // 忽略这棵子树，继续访问其他节点
	T_CHK_STOP, // 终止访问循环
	T_CHK_DESTROY // 销毁这棵子树
}CTCheckResult;

typedef CTCheckResult (*CheckTreeFunc)(CMallocator& stMalloc, uint8_t uchIndexCnt, uint8_t uchCurIdxCnt, const CRowValue* pstIndexValue, const uint32_t uiTreeRowNum, void* pCookie);
typedef int (*VisitRawData)(CMallocator& stMalloc, uint8_t uchIndexCnt, const CRowValue* pstIndexValue, ALLOC_HANDLE_T& hHandle, int64_t& llRowNumInc, void* pCookie);

/************************************************************
  Description:    t-tree根节点的数据结构
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
	T_COND_VAL_SET, // 查询特定的值列表
	T_COND_RANGE, // 查询value[0] ~ Key-value[0]<=value[1].s64
	T_COND_GE, // 查询大于等于value[0]的key
	T_COND_LE, // 查询小于等于value[0]的key
	T_COND_ALL // 遍历所有key
}CTCondType;

typedef enum _CTOrder{
	T_ORDER_ASC, // 升序
	T_ORDER_DEC, // 降序
	T_ORDER_POS, // 后序访问
}CTOrder;

/************************************************************
  Description:    查找数据的条件
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
	CRootData* m_pstRootData; // 注意：地址可能会因为realloc而改变
	CTtree	m_stTree;
	const CTableDefinition* m_pstTab;
	uint8_t m_uchIndexDepth;
	char	m_szErr[100];
	
	ALLOC_SIZE_T	m_uiNeedSize; // 最近一次分配内存失败需要的大小
	
	MEM_HANDLE_T	_handle;
    uint64_t    	_size;
    CMallocator*		_mallocator;
	
	/************************************************************
	  Description:    递归查找数据的cookie参数
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
	  Description:	新分配一块内存，并初始化
	  Input:		 iKeySize	key的格式，0为变长，非0为定长长度
				pchKey	为格式化后的key，变长key的第0字节为长度
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Init(int iKeySize, const char* pchKey);
	
	const char* Key() const { return m_pstRootData?m_pstRootData->m_achKey:NULL; }
	char* Key(){ return m_pstRootData?m_pstRootData->m_achKey:NULL; }
	
	unsigned int  TotalRows(){ return m_pstRootData->m_uiRowCnt; }
	
	/*************************************************
	  Description:	最近一次分配内存失败所需要的内存大小
	  Input:		
	  Output:		
	  Return:		返回所需要的内存大小
	*************************************************/
	ALLOC_SIZE_T NeedSize() { return m_uiNeedSize; }
	
	/*************************************************
	  Description:	销毁uchLevel以及以下级别的子树
	  Input:		uchLevel	销毁uchLevel以及以下级别的子树，显然uchLevel应该在1到uchIndexDepth之间
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
//	int Destroy(uint8_t uchLevel=1);
	int Destroy();
	
	/*************************************************
	  Description:	插入一行数据
	  Input:		stRow	包含index字段以及后面字段的值
				pfComp	用户自定义的key比较函数
				uchFlag	行标记
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int InsertRowFlag(const CRowValue& stRow, KeyComparator pfComp, unsigned char uchFlag);
	
	/*************************************************
	  Description:	插入一行数据
	  Input:		stRow	包含index字段以及后面字段的值
				pfComp	用户自定义的key比较函数
				isDirty	是否脏数据
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int InsertRow(const CRowValue& stRow, KeyComparator pfComp, bool isDirty);
	
	/*************************************************
	  Description:	查找一行数据
	  Input:		stCondition	包含各级index字段的值
				pfComp	用户自定义的key比较函数
				
	  Output:		hRecord	查找到的一个指向CRawData的句柄
	  Return:		0为找不到，1为找到数据
	*************************************************/
	int Find(const CRowValue& stCondition, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	
	/*************************************************
	  Description:	按索引条件查找
	  Input:		pstCond	一个数组，而且大小刚好是uchIndexDepth
				pfComp	用户自定义的key比较函数
				pfVisit	当查找到记录时，用户自定义的访问数据函数
				pCookie	访问数据函数使用的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int Search(const CTtreeCondition* pstCond, KeyComparator pfComp, VisitRawData pfVisit, CheckTreeFunc pfCheck, void* pCookie);
	
	/*************************************************
	  Description:	从小到大遍历所有数据
	  Input:		pfComp	用户自定义的key比较函数
				pfVisit	当查找到记录时，用户自定义的访问数据函数
				pCookie	访问数据函数使用的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseForward(KeyComparator pfComp, VisitRawData pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	根据指定的index值，删除符合条件的所有行（包括子树）
	  Input:		uchCondIdxCnt	条件index的数量
				stCondition		包含各级index字段的值
				pfComp		用户自定义的key比较函数
				
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int Delete(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp);
	
	/*************************************************
	  Description:	将某个级别的index值修改为另外一个值
	  Input:		uchCondIdxCnt	条件index的数量
				stCondition		包含各级index字段的值
				pfComp		用户自定义的key比较函数
				pstNewValue	对应最后一个条件字段的新index值
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int UpdateIndex(uint8_t uchCondIdxCnt, const CRowValue& stCondition, KeyComparator pfComp, const CValue* pstNewValue);
	unsigned AskForDestroySize(void);
};

#endif
