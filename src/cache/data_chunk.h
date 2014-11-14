#ifndef DATA_CHUNK_H
#define DATA_CHUNK_H

/************************************************************
  Description:    封装了数据Chunk的操作      
  Version:         TTC 3.0
  Function List:   
    1.  BaseSize()		计算基本结构大小
   2.  Key()			获取格式化后的key
   3. SetKey() 		保存key
   4. StrKeySize()	获取字符串key的实际长度
   5. Destroy()		销毁并释放内存
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include <stdint.h>
#include "raw_data.h"
#include "tree_data.h"

class CDataChunk{
protected:
	unsigned char	m_uchDataType; // 数据chunk的类型
	
public:
	/*************************************************
	  Description:	计算基本结构大小
	  Input:		
	  Output:		
	  Return:		内存大小
	*************************************************/
	ALLOC_SIZE_T BaseSize()
	{
		if(m_uchDataType == DATA_TYPE_RAW)
			return(sizeof(CRawFormat));
		else
			return(sizeof(CRootData));
	}
	
	/*************************************************
	  Description:	获取格式化后的key
	  Input:		
	  Output:		
	  Return:		key指针
	*************************************************/
	const char* Key()const
	{
		if(m_uchDataType == DATA_TYPE_RAW){
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_achKey;
		}
		else if(m_uchDataType == DATA_TYPE_TREE_ROOT){
			CRootData* pstRoot = (CRootData*)this;
			return pstRoot->m_achKey;
		}
		return NULL;
	}
	
	/*************************************************
	  Description:	获取格式化后的key
	  Input:		
	  Output:		
	  Return:		key指针
	*************************************************/
	char* Key()
	{
		if(m_uchDataType == DATA_TYPE_RAW){
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_achKey;
		}
		else if(m_uchDataType == DATA_TYPE_TREE_ROOT){
			CRootData* pstRoot = (CRootData*)this;
			return pstRoot->m_achKey;
		}
		return NULL;
	}
	
	/*************************************************
	  Description:	保存key
	  Input:		key	key的实际值
	  Output:		
	  Return:		
	*************************************************/
			
#define SET_KEY_FUNC(type, key) void SetKey(type key) \
	{ \
		if(m_uchDataType == DATA_TYPE_RAW){ \
			CRawFormat* pstRaw = (CRawFormat*)this; \
			*(type*)(void *)pstRaw->m_achKey = key; \
		} \
		else{ \
			CRootData* pstRoot = (CRootData*)this; \
			*(type*)(void *)pstRoot->m_achKey = key; \
		} \
	}
	
	SET_KEY_FUNC(int32_t, iKey)
	SET_KEY_FUNC(uint32_t, uiKey)
	SET_KEY_FUNC(int64_t, llKey)
	SET_KEY_FUNC(uint64_t, ullKey)
	
	/*************************************************
	  Description:	保存字符串key
	  Input:		key	key的实际值
				iLen	key的长度
	  Output:		
	  Return:		
	*************************************************/
	void SetKey(const char* pchKey, int iLen)
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			*(unsigned char*)pstRaw->m_achKey = iLen;
			memcpy(pstRaw->m_achKey+1, pchKey, iLen);
		} 
		else{ 
			CRootData* pstRoot = (CRootData*)this; 
			*(unsigned char*)pstRoot->m_achKey = iLen;
			memcpy(pstRoot->m_achKey+1, pchKey, iLen);
		} 
	}
	
	/*************************************************
	  Description:	保存格式化好的字符串key
	  Input:		key	key的实际值, 要求key[0]是长度
	  Output:		
	  Return:		
	*************************************************/
	void SetKey(const char* pchKey)
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			memcpy(pstRaw->m_achKey, pchKey, *(unsigned char*)pchKey);
		} 
		else{ 
			CRootData* pstRoot = (CRootData*)this; 
			memcpy(pstRoot->m_achKey, pchKey, *(unsigned char*)pchKey);
		} 
	}
	
	/*************************************************
	  Description:	查询字符串key大小
	  Input:		
	  Output:		
	  Return:		key大小
	*************************************************/
	int StrKeySize()
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			return *(unsigned char*)pstRaw->m_achKey;
		} 
		else{ 
			CRootData* pstRoot = (CRootData*)this; 
			return *(unsigned char*)pstRoot->m_achKey;
		} 
	}
	
	/*************************************************
	  Description:	查询二进制key大小
	  Input:		
	  Output:		
	  Return:		key大小
	*************************************************/
	int BinKeySize(){ return StrKeySize(); }
	
	unsigned int HeadSize()
	{
		if(m_uchDataType == DATA_TYPE_RAW)
			return sizeof(CRawFormat);
		else
			return sizeof(CRootData);
	}
	
	/*************************************************
	  Description:	查询数据头大小，如果是CRawData的chunk，DataSize()是不包括Row的长度，仅包括头部信息以及key
	  Input:		
	  Output:		
	  Return:		内存大小
	*************************************************/
	unsigned int DataSize(int iKeySize)
	{
		int iKeyLen = iKeySize? iKeySize : 1+StrKeySize();
		return HeadSize() + iKeyLen;
	}
	
	unsigned int NodeSize()
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_uiDataSize;
		} 
		else{ 
			return 0; // unknow
		} 
	}
	
	unsigned int CreateTime()
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_CreateHour;
		} 
		else{ 
			return 0; // unknow
		} 
	}
	unsigned LastAccessTime()
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_LastAccessHour;
		} 
		else{ 
			return 0; // unknow
		} 
	}
	unsigned int LastUpdateTime()
	{
		if(m_uchDataType == DATA_TYPE_RAW){ 
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_LastUpdateHour;
		} 
		else{ 
			return 0; // unknow
		} 
	}
	
	uint32_t TotalRows()
	{
		if(m_uchDataType == DATA_TYPE_RAW){
			CRawFormat* pstRaw = (CRawFormat*)this;
			return pstRaw->m_uiRowCnt;
		}
		else{
			CRootData* pstRoot = (CRootData*)this;
			return pstRoot->m_uiRowCnt;
		}
	}
	
	/*************************************************
	  Description:	销毁内存并释放内存
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Destroy(CMallocator* pstMalloc)
	{
		MEM_HANDLE_T hHandle = pstMalloc->Ptr2Handle(this);
		if(m_uchDataType == DATA_TYPE_RAW){
			return pstMalloc->Free(hHandle);
		}
#if HAS_TREE_DATA
		else if(m_uchDataType == DATA_TYPE_TREE_ROOT){
			CTreeData stTree(pstMalloc, NULL);
			int iRet = stTree.Attach(hHandle);
			if(iRet != 0){
				return(iRet);
			}
			return stTree.Destroy();
		}
#endif
		
		return(-1);
	}

	/* 查询如果destroy这块内存，能释放多少空间出来 （包括合并）*/
	unsigned AskForDestroySize(CMallocator* pstMalloc)
	{
		MEM_HANDLE_T hHandle = pstMalloc->Ptr2Handle(this);

		if(m_uchDataType == DATA_TYPE_RAW){
			return pstMalloc->AskForDestroySize(hHandle);
		}
#if HAS_TREE_DATA
		else if(m_uchDataType == DATA_TYPE_TREE_ROOT){
			CTreeData stTree(pstMalloc, NULL);
			if(stTree.Attach(hHandle))
				return 0;

			return stTree.AskForDestroySize();
		}
#endif

		log_debug("AskForDestroySize failed");
		return 0;
	}

};

#endif
