#ifndef DATA_CHUNK_H
#define DATA_CHUNK_H

/************************************************************
  Description:    ��װ������Chunk�Ĳ���      
  Version:         TTC 3.0
  Function List:   
    1.  BaseSize()		��������ṹ��С
   2.  Key()			��ȡ��ʽ�����key
   3. SetKey() 		����key
   4. StrKeySize()	��ȡ�ַ���key��ʵ�ʳ���
   5. Destroy()		���ٲ��ͷ��ڴ�
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include <stdint.h>
#include "raw_data.h"
#include "tree_data.h"

class CDataChunk{
protected:
	unsigned char	m_uchDataType; // ����chunk������
	
public:
	/*************************************************
	  Description:	��������ṹ��С
	  Input:		
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	ALLOC_SIZE_T BaseSize()
	{
		if(m_uchDataType == DATA_TYPE_RAW)
			return(sizeof(CRawFormat));
		else
			return(sizeof(CRootData));
	}
	
	/*************************************************
	  Description:	��ȡ��ʽ�����key
	  Input:		
	  Output:		
	  Return:		keyָ��
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
	  Description:	��ȡ��ʽ�����key
	  Input:		
	  Output:		
	  Return:		keyָ��
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
	  Description:	����key
	  Input:		key	key��ʵ��ֵ
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
	  Description:	�����ַ���key
	  Input:		key	key��ʵ��ֵ
				iLen	key�ĳ���
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
	  Description:	�����ʽ���õ��ַ���key
	  Input:		key	key��ʵ��ֵ, Ҫ��key[0]�ǳ���
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
	  Description:	��ѯ�ַ���key��С
	  Input:		
	  Output:		
	  Return:		key��С
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
	  Description:	��ѯ������key��С
	  Input:		
	  Output:		
	  Return:		key��С
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
	  Description:	��ѯ����ͷ��С�������CRawData��chunk��DataSize()�ǲ�����Row�ĳ��ȣ�������ͷ����Ϣ�Լ�key
	  Input:		
	  Output:		
	  Return:		�ڴ��С
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
	  Description:	�����ڴ沢�ͷ��ڴ�
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
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

	/* ��ѯ���destroy����ڴ棬���ͷŶ��ٿռ���� �������ϲ���*/
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
