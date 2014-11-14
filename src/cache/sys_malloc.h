#ifndef SYS_MALLOC_H
#define SYS_MALLOC_H

/************************************************************
  Description:   ��ϵͳ�ڴ�����װ�����Ƕ�����ڴ������
  Version:         TTC 3.0
   Function List:   
   1. Malloc()		�����ڴ�
   2. Calloc()		�����ڴ棬����ʼ��Ϊ0
   3. ReAlloc()		���·����ڴ�
   4. Free()			�ͷ��ڴ�
   5.ChunkSize()		��ȡ�ڴ��Ĵ�С
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "namespace.h"
#include "mallocator.h"

TTC_BEGIN_NAMESPACE

class CSysMalloc: public CMallocator
{
private:
	char m_szErr[200];
	
public:
	CSysMalloc(){}
	virtual ~CSysMalloc(){}
	
	template <class T>
	    T * Pointer(ALLOC_HANDLE_T hHandle){return reinterpret_cast<T *>(Handle2Ptr(hHandle));}
	
	ALLOC_HANDLE_T Handle(void *p){ return (ALLOC_HANDLE_T)((char*)p - (char*)0); } 

	const char* GetErrMsg(){ return m_szErr; }
	
	/*************************************************
	  Description:	�����ڴ�
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	ALLOC_HANDLE_T Malloc(ALLOC_SIZE_T tSize)
	{
		void* p = malloc(sizeof(ALLOC_SIZE_T)+tSize);
		if(p == NULL){
			snprintf(m_szErr, sizeof(m_szErr), "%m");
			return(INVALID_HANDLE);
		}
		*(ALLOC_SIZE_T*)p = tSize;
		return Handle((void*)((char*)p+sizeof(ALLOC_SIZE_T)));
	}

	/*************************************************
	  Description:	�����ڴ棬�����ڴ��ʼ��Ϊ0
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	ALLOC_HANDLE_T Calloc(ALLOC_SIZE_T tSize)
	{
		void* p = calloc(1, sizeof(ALLOC_SIZE_T)+tSize);
		if(p == NULL){
			snprintf(m_szErr, sizeof(m_szErr), "%m");
			return(INVALID_HANDLE);
		}
		*(ALLOC_SIZE_T*)p = tSize;
		return Handle((void*)((char*)p+sizeof(ALLOC_SIZE_T)));
	}

	/*************************************************
	  Description:	���·����ڴ�
	  Input:		hHandle	���ڴ���
				tSize		�·�����ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��(ʧ��ʱ�����ͷ����ڴ��)
	*************************************************/
	ALLOC_HANDLE_T ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize)
	{
		char* old;
		if(hHandle == INVALID_HANDLE)
			old = NULL;
		else
			old = (char*)0+(hHandle-sizeof(ALLOC_SIZE_T));
		if(tSize == 0){
			free(old);
			return(INVALID_HANDLE);
		}
		void* p = realloc(old, sizeof(ALLOC_SIZE_T)+tSize);
		if(p == NULL){
			snprintf(m_szErr, sizeof(m_szErr), "%m");
			return(INVALID_HANDLE);
		}
		*(ALLOC_SIZE_T*)p = tSize;
		return Handle((void*)((char*)p+sizeof(ALLOC_SIZE_T)));
	}
	
	/*************************************************
	  Description:	�ͷ��ڴ�
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Free(ALLOC_HANDLE_T hHandle)
	{
		if(hHandle == INVALID_HANDLE)
			return(0);
			
		char* old = (char*)0+(hHandle-sizeof(ALLOC_SIZE_T));
		free(old);
		return(0);
	}
	
	/*************************************************
	  Description:	��ȡ�ڴ���С
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	ALLOC_SIZE_T ChunkSize(ALLOC_HANDLE_T hHandle)
	{
		if(hHandle == INVALID_HANDLE)
			return(0);
		
		char* old = (char*)0+(hHandle-sizeof(ALLOC_SIZE_T));
		return *(ALLOC_SIZE_T*)old;
	}
	
	/*************************************************
	  Description:	�����ת�����ڴ��ַ
	  Input:		�ڴ���
	  Output:		
	  Return:		�ڴ��ַ����������Ч����NULL
	*************************************************/
	void* Handle2Ptr(ALLOC_HANDLE_T hHandle)
	{
		return (char*)0+hHandle;
	}
	
	/*************************************************
	  Description:	���ڴ��ַת��Ϊ���
	  Input:		�ڴ��ַ
	  Output:		
	  Return:		�ڴ����������ַ��Ч����INVALID_HANDLE
	*************************************************/
	ALLOC_HANDLE_T Ptr2Handle(void* p)
	{
		return Handle(p);
	}

	/* not implement */
	ALLOC_SIZE_T AskForDestroySize(ALLOC_HANDLE_T hHandle)
	{
		return (ALLOC_SIZE_T)0;
	}

    /*************************************************
	  Description:	���handle�Ƿ���Ч
	  Input:		�ڴ���
	  Output:		
      Return:	    0: ��Ч; -1:��Ч
	*************************************************/
    virtual int HandleIsValid (ALLOC_HANDLE_T mem_handle)
    {
        return 0;
    }
};


extern CSysMalloc g_stSysMalloc;

TTC_END_NAMESPACE

#endif
