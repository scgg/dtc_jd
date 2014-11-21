#ifndef T_TREE_H
#define T_TREE_H

/************************************************************
  Description:    封装了T-tree的各种操作      
  Version:         TTC 3.0
  Function List:   
    1.  Attach()		attach一颗树
   2.  Insert()		插入一条记录
   3. Delete() 		删除一条记录
   4. Find()			查找一条记录
   5. TraverseForward()	从小到大遍历整棵树
   6. TraverseBackward()	从大到小遍历整棵树
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include <stdint.h>
#include "mallocator.h"

//TTC_BEGIN_NAMESPACE
#ifdef MODU_TEST
class CDataChunk{
public:
	int m_iValue;
	
	const char* Key(){ return (const char*)&m_iValue; }
};
#endif

typedef int64_t (*KeyComparator)(const char* pchKey, void* pCmpCookie, CMallocator& stMalloc, ALLOC_HANDLE_T hOtherKey);
typedef int (*ItemVisit)(CMallocator& stMalloc, ALLOC_HANDLE_T& hRecord, void* pCookie);


class CTtree{
protected:
	ALLOC_HANDLE_T	m_hRoot;
	CMallocator&	m_stMalloc;
	char m_szErr[100];
	
public:
	CTtree(CMallocator& stMalloc);
	~CTtree();
	
	const char* GetErrMsg(){ return m_szErr; }
	const ALLOC_HANDLE_T Root() const { return m_hRoot; }
	
	/*************************************************
	  Description:	attach一块已经格式化好的内存
	  Input:		
	  Output:		
	  Return:		
	*************************************************/
	void Attach(ALLOC_HANDLE_T hRoot){ m_hRoot = hRoot; }
	
	/*************************************************
	  Description:	将key insert到树里，hRecord为key对应的数据（包含key）
	  Input:		pchKey		插入的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				hRecord		保存着要插入的key以及其他数据的句柄
	  Output:		
	  Return:		0为成功，EC_NO_MEM为内存不足，EC_KEY_EXIST为key已经存在，其他值为错误
	*************************************************/
	int Insert(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T hRecord);
	
	/*************************************************
	  Description:	删除key以及对应的数据(但不会自动释放key对应的内存)
	  Input:		pchKey		插入的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int Delete(const char* pchKey, void* pCmpCookie, KeyComparator pfComp);
	
	int FindHandle(ALLOC_HANDLE_T hRecord);
	
	/*************************************************
	  Description:	查找key对应的数据
	  Input:		pchKey		插入的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
	  Output:		hRecord		保存查找到的key以及其他数据的句柄
	  Return:		0为查找不到，1为找到数据
	*************************************************/
	int Find(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	
	/*************************************************
	  Description:	查找key对应的数据
	  Input:		pchKey		插入的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
	  Output:		phRecord		指向树节点的item指针
	  Return:		0为查找不到，1为找到数据
	*************************************************/
	int Find(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T*& phRecord);
	
	/*************************************************
	  Description:	销毁整棵树，并释放相应的内存
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Destroy();

	/*************************************************
	  Description: 查询销毁整棵树可以释放多少空闲内存	
	  Input:		
	  Output:		
	  Return:	 >0 成功， 0 失败
	*************************************************/
	unsigned AskForDestroySize(void);
	
	/*************************************************
	  Description:	从小到大遍历整棵树
	  Input:		pfVisit	访问数据记录的用户自定义函数
				pCookie	自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseForward(ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从大到小遍历整棵树
	  Input:		pfVisit	访问数据记录的用户自定义函数
				pCookie	自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseBackward(ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	后序遍历整棵树
	  Input:		pfVisit	访问数据记录的用户自定义函数
				pCookie	自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int PostOrderTraverse(ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，从小到大遍历树，遍历的范围为[key, key+iInclusion]
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				iInclusion		key的范围
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseForward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, int64_t iInclusion, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，从小到大遍历树, 遍历的范围为[key, key1]
	  Input:		pchKey		开始的key
				pchKey1		结束的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseForward(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，从小到大遍历树(遍历大于等于key的所有记录)
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseForward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，从大到小遍历树(遍历小于等于key的所有记录)
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseBackward(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，从大到小遍历树，遍历的范围为[key, key1]
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int TraverseBackward(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);

	/*************************************************
	  Description:	从指定的key开始，先左右树，后根结点, 遍历的范围为[key, key1]
	  Input:		pchKey		开始的key
				pchKey1		结束的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int PostOrderTraverse(const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，后序遍历树(遍历大于等于key的所有记录)
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int PostOrderTraverseGE(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	/*************************************************
	  Description:	从指定的key开始，后序遍历树(遍历小于等于key的所有记录)
	  Input:		pchKey		开始的key
				pCmpCookie	调用用户自定义的pfComp函数跟树里的节点比较时作为输入参数
				pfComp		用户自定义的key比较函数
				pfVisit		访问数据记录的用户自定义函数
				pCookie		自定义函数的cookie参数
	  Output:		
	  Return:		0为成功，其他值为错误
	*************************************************/
	int PostOrderTraverseLE(const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
};

/************************************************************
  Description:    封装了T-tree node的各种操作，仅供t-tree内部使用   
  Version:         TTC 3.0
***********************************************************/
struct _CTtreeNode
{
	enum { 
//       	 PAGE_SIZE = 125,
		PAGE_SIZE = 20,		// 每个节点保存多少条记录
        MIN_ITEMS = PAGE_SIZE - 2 // minimal number of items in internal node
    };

	ALLOC_HANDLE_T	m_hLeft;
	ALLOC_HANDLE_T	m_hRight;
	int8_t			m_chBalance;
	uint16_t		m_ushNItems;
	ALLOC_HANDLE_T	m_ahItems[PAGE_SIZE];
	
	int Init();
	static ALLOC_HANDLE_T Alloc(CMallocator& stMalloc, ALLOC_HANDLE_T hRecord);
	static int Insert(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T hRecord);
	static int Delete(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode, const char* pchKey, void* pCmpCookie, KeyComparator pfComp);
	static int BalanceLeftBranch(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode);
	static int BalanceRightBranch(CMallocator& stMalloc, ALLOC_HANDLE_T& hNode);
	static int Destroy(CMallocator& stMalloc, ALLOC_HANDLE_T hNode);
	static unsigned AskForDestroySize(CMallocator&, ALLOC_HANDLE_T hNode); 
	
	// 查找指定的key。找到返回1，否则返回0
	int Find(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hRecord);
	int Find(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T*& phRecord);
	int FindHandle(CMallocator& stMalloc, ALLOC_HANDLE_T hRecord);
	// 假设node包含key-k1~kn，查找这样的node节点：k1<= key <=kn
	int FindNode(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ALLOC_HANDLE_T& hNode);
	int TraverseForward(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie);
	int TraverseBackward(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie);
	int PostOrderTraverse(CMallocator& stMalloc, ItemVisit pfVisit, void* pCookie);
	
	int TraverseForward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, int iInclusion, ItemVisit pfVisit, void* pCookie);
	int TraverseForward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	int TraverseForward(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	int TraverseBackward(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	int TraverseBackward(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	
	int PostOrderTraverse(CMallocator& stMalloc, const char* pchKey, const char* pchKey1, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	int PostOrderTraverseGE(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
	int PostOrderTraverseLE(CMallocator& stMalloc, const char* pchKey, void* pCmpCookie, KeyComparator pfComp, ItemVisit pfVisit, void* pCookie);
}__attribute__((packed));
typedef struct _CTtreeNode CTtreeNode;


//TTC_END_NAMESPACE

#endif
