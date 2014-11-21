#ifndef RAW_DATA_H
#define RAW_DATA_H

/************************************************************
  Description:    封装了平板数据结构存储操作      
  Version:         TTC 3.0
  Function List:   
    1.  Init()			初始化数据
   2.  InsertRow()		插入一行数据
   3. DecodeRow() 	读取一行数据
   4. ReplaceCurRow()	替换当前行
   5. DeleteCurRow()	删除当前行
   6. CopyRow()		从Refrence里copy当前行
   7. CopyAll()		从refrence里copy所有数据
   8. AppendNRecords()	添加N行已经格式化好的数据
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include "bin_malloc.h"
#include "global.h"
#include "field.h"

#define PRE_DECODE_ROW 1

typedef enum _CDataType{
	DATA_TYPE_RAW,			// 平板数据结构
	DATA_TYPE_TREE_ROOT,	// 树的根节点
	DATA_TYPE_TREE_NODE		// 树的节点
}CDataType;

typedef enum _enum_oper_type_
{
    OPER_DIRTY	    = 0x02,	// cover INSERT, DELETE, UPDATE
    OPER_SELECT     = 0x30,
    OPER_INSERT_OLD = 0x31,	// old stuff, same as SELECT aka useless
    OPER_UPDATE     = 0x32,
    OPER_DELETE_NA  = 0x33,	// async DELETE require quite a lot change
    OPER_FLUSH      = 0x34,	// useless too, same as SELECT
    OPER_RESV1      = 0x35,
    OPER_INSERT     = 0x36,
    OPER_RESV2      = 0x37,
} TOperType;

struct CRawFormat{
	unsigned char	m_uchDataType;	// 数据类型CDataType
	uint32_t m_uiDataSize;			// 数据总大小
	uint32_t m_uiRowCnt;			// 行数
	char	m_achKey[0];			// key
	char	m_achRows[0];			// 行数据
}__attribute__((packed));


// 注意：修改操作可能会导致handle改变，因此需要检查重新保存
class CRawData
{
private:
	char*	m_pchContent; // 注意：地址可能会因为realloc而改变
	uint32_t	m_uiDataSize; // 包括data_type,data_size,rowcnt,key,rows等总数据大小
	uint32_t	m_uiRowCnt;
	uint8_t		m_uchKeyIdx;
	int		m_iKeySize;
	int		m_iLAId;
	ALLOC_SIZE_T	m_uiKeyStart;
	ALLOC_SIZE_T	m_uiDataStart;
	ALLOC_SIZE_T	m_uiRowOffset;
	ALLOC_SIZE_T	m_uiOffset;
	ALLOC_SIZE_T	m_uiLAOffset;

	ALLOC_SIZE_T	m_uiNeedSize; // 最近一次分配内存失败需要的大小
	
	MEM_HANDLE_T	_handle;
	uint64_t    	_size;
	CMallocator*	_mallocator;
	int		_autodestroy;
	
	CRawData* m_pstRef;
	char	m_szErr[200];

protected:
	template <class T>
	T* Pointer(void) const {return reinterpret_cast<T *>(_mallocator->Handle2Ptr(_handle));}
	
	int SetDataSize();
	int SetRowCount();
	int ExpandChunk(ALLOC_SIZE_T tExpSize);
	int ReAllocChunk(ALLOC_SIZE_T tSize);
	int SkipRow(const CRowValue &stRow);
	int EncodeRow(const CRowValue &stRow, unsigned char uchOp, bool expendBuf=true);
	
public:

	/*************************************************
	  Description:    构造函数
	  Input:          pstMalloc	内存分配器
	                     iAutoDestroy	析构的时候是否自动释放内存
	  Output:         
	  Return:         
	*************************************************/
	CRawData(CMallocator* pstMalloc, int iAutoDestroy=0);
	
	~CRawData();
	
	const char* GetErrMsg(){ return m_szErr; }
	
	/*************************************************
	  Description:	新分配一块内存，并初始化
	  Input:		 uchKeyIdx	作为key的字段在table里的下标
				iKeySize	key的格式，0为变长，非0为定长长度
				pchKey	为格式化后的key，变长key的第0字节为长度
				uiDataSize	为数据的大小，用于一次分配足够大的chunk。如果设置为0，则insert row的时候再realloc扩大
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Init(uint8_t uchKeyIdx, int iKeySize, const char* pchKey, ALLOC_SIZE_T uiDataSize=0, int laid=-1);
	
	/*************************************************
	  Description:	attach一块已经格式化好的内存
	  Input:		hHandle	内存的句柄
				uchKeyIdx	作为key的字段在table里的下标
				iKeySize	key的格式，0为变长，非0为定长长度
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Attach(MEM_HANDLE_T hHandle, uint8_t uchKeyIdx, int iKeySize, int laid=-1);
	
	/*************************************************
	  Description:	获取内存块的句柄
	  Input:		
	  Output:		
	  Return:		句柄。 注意：任何修改操作可能会导致handle改变，因此需要检查重新保存
	*************************************************/
	MEM_HANDLE_T GetHandle(){ return _handle; }
	
	const char* GetAddr() const { return m_pchContent; }
	
	/*************************************************
	  Description:	设置一个refrence，在调用CopyRow()或者CopyAll()的时候使用
	  Input:		pstRef	refrence指针
	  Output:		
	  Return:		
	*************************************************/
	void SetRefrence(CRawData* pstRef){ m_pstRef = pstRef; }
	
	/*************************************************
	  Description:	包括key、rows等所有内存的大小
	  Input:		
	  Output:		
	  Return:		所有内存的大小
	*************************************************/
	uint32_t DataSize() const { return m_uiDataSize; }
	
	/*************************************************
	  Description:	rows的开始偏移量
	  Input:		
	  Output:		
	  Return:		rows的开始偏移量
	*************************************************/
	uint32_t DataStart() const { return m_uiDataStart; }
	
	/*************************************************
	  Description:	内存分配失败时，返回所需要的内存大小
	  Input:		
	  Output:		
	  Return:		返回所需要的内存大小
	*************************************************/
	ALLOC_SIZE_T NeedSize() { return m_uiNeedSize; }
	
	/*************************************************
	  Description:	计算插入该行所需要的内存大小
	  Input:		stRow	行数据
	  Output:		
	  Return:		返回所需要的内存大小
	*************************************************/
	ALLOC_SIZE_T CalcRowSize(const CRowValue &stRow);
	
	/*************************************************
	  Description:	获取格式化后的key
	  Input:		
	  Output:		
	  Return:		格式化后的key
	*************************************************/
	const char* Key() const { return m_pchContent?(m_pchContent+m_uiKeyStart):NULL; }
	char* Key(){ return m_pchContent?(m_pchContent+m_uiKeyStart):NULL; }
	
	/*************************************************
	  Description:	获取key的格式
	  Input:		
	  Output:		
	  Return:		变长返回0，定长key返回定长的长度
	*************************************************/
	int KeyFormat() const { return m_iKeySize; }
	
	/*************************************************
	  Description:	获取key的实际长度
	  Input:		
	  Output:		
	  Return:		key的实际长度
	*************************************************/
	int KeySize();
	 
	unsigned int  TotalRows() const { return m_uiRowCnt; }
	void Rewind(void) { m_uiOffset = m_uiDataStart; m_uiRowOffset = m_uiDataStart;}
	
	/*************************************************
	  Description:	销毁释放内存
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Destroy();
	
	/*************************************************
	  Description:	释放多余的内存（通常在delete一些row后调用一次）
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int StripMem();
	
	/*************************************************
	  Description:	读取一行数据
	  Input:		
	  Output:		stRow	保存行数据
				uchRowFlags	行数据是否脏数据等flag
				iDecodeFlag	是否只是pre-read，不fetch_row移动指针
	  Return:		0为成功，非0失败
	*************************************************/
	int DecodeRow(CRowValue &stRow, unsigned char& uchRowFlags, int iDecodeFlag=0);
	
	/*************************************************
	  Description:	插入一行数据
	  Input:		stRow	需要插入的行数据
	  Output:		
				byFirst	是否插入到最前面，默认添加到最后面
				isDirty	是否脏数据
	  Return:		0为成功，非0失败
	*************************************************/
	int InsertRow(const CRowValue &stRow, bool byFirst, bool isDirty);
	
	/*************************************************
	  Description:	插入一行数据
	  Input:		stRow	需要插入的行数据
	  Output:		
				byFirst	是否插入到最前面，默认添加到最后面
				uchOp	row的标记
	  Return:		0为成功，非0失败
	*************************************************/
	int InsertRowFlag(const CRowValue &stRow, bool byFirst, unsigned char uchOp);
	
	/*************************************************
	  Description:	插入若干行数据
	  Input:		uiNRows	行数
				stRow	需要插入的行数据
	  Output:		
				byFirst	是否插入到最前面，默认添加到最后面
				isDirty	是否脏数据
	  Return:		0为成功，非0失败
	*************************************************/
	int InsertNRows(unsigned int uiNRows, const CRowValue* pstRow, bool byFirst, bool isDirty);
	
	/*************************************************
	  Description:	用指定数据替换当前行
	  Input:		stRow	新的行数据
	  Output:		
				isDirty	是否脏数据
	  Return:		0为成功，非0失败
	*************************************************/
	int ReplaceCurRow(const CRowValue &stRow, bool isDirty);
	
	/*************************************************
	  Description:	删除当前行
	  Input:		stRow	仅使用row的字段类型等信息，不需要实际数据
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int DeleteCurRow(const CRowValue& stRow);
	
	/*************************************************
	  Description:	删除所有行
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int DeleteAllRows();
	
	/*************************************************
	  Description:	设置当前行的标记
	  Input:		uchFlag	行的标记
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int SetCurRowFlag(unsigned char uchFlag);
	
	/*************************************************
	  Description:	从refrence copy当前行到本地buffer末尾
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int CopyRow();
	
	/*************************************************
	  Description:	用refrence的数据替换本地数据
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int CopyAll();
	
	/*************************************************
	  Description:	添加N行已经格式化好的数据到末尾
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int AppendNRecords(unsigned int uiNRows, const char* pchData, const unsigned int uiLen);

	/*************************************************
	  Description:	更新最后访问时间戳
	  Input:	时间戳	
	  Output:		
	  Return:
	*************************************************/
	void UpdateLastacc(uint32_t now) {
		if(m_uiLAOffset > 0) *(uint32_t *)(m_pchContent + m_uiLAOffset) = now;
	}
};

inline int CRawData::KeySize()
{
	return m_iKeySize > 0 ? m_iKeySize: (sizeof(char)+*(unsigned char*)Key());
}

#endif
