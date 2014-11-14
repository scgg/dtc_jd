#ifndef MALLOCATOR_H
#define MALLOCATOR_H

/************************************************************
  Description:   �������ڴ�������ĸ��ֽӿ�(��������)
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
#include "namespace.h"

TTC_BEGIN_NAMESPACE

#define ALLOC_SIZE_T uint32_t
#define ALLOC_HANDLE_T uint64_t
#define INTER_SIZE_T uint64_t
#define INTER_HANDLE_T uint64_t

#define INVALID_HANDLE 0ULL

#define SIZE_SZ (sizeof(ALLOC_SIZE_T))
#define MALLOC_ALIGNMENT (2 * SIZE_SZ)
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)
#define MAX_ALLOC_SIZE (((ALLOC_SIZE_T)-1) & ~MALLOC_ALIGN_MASK)

class CMallocator{
public:
	CMallocator(){}
	virtual ~CMallocator(){}
	
	template <class T>
	    T * Pointer(ALLOC_HANDLE_T hHandle){return reinterpret_cast<T *>(Handle2Ptr(hHandle));}
	
	virtual ALLOC_HANDLE_T Handle(void *p)=0;

	virtual const char* GetErrMsg()=0;
	
	/*************************************************
	  Description:	�����ڴ�
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	virtual ALLOC_HANDLE_T Malloc(ALLOC_SIZE_T tSize)=0;

	/*************************************************
	  Description:	�����ڴ棬�����ڴ��ʼ��Ϊ0
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	virtual ALLOC_HANDLE_T Calloc(ALLOC_SIZE_T tSize)=0;

	/*************************************************
	  Description:	���·����ڴ�
	  Input:		hHandle	���ڴ���
				tSize		�·�����ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��(ʧ��ʱ�����ͷ����ڴ��)
	*************************************************/
	virtual ALLOC_HANDLE_T ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize)=0;
	
	/*************************************************
	  Description:	�ͷ��ڴ�
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	virtual int Free(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	��ȡ�ڴ���С
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	virtual ALLOC_SIZE_T ChunkSize(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	�����ת�����ڴ��ַ
	  Input:		�ڴ���
	  Output:		
	  Return:		�ڴ��ַ����������Ч����NULL
	*************************************************/
	virtual void* Handle2Ptr(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	���ڴ��ַת��Ϊ���
	  Input:		�ڴ��ַ
	  Output:		
	  Return:		�ڴ����������ַ��Ч����INVALID_HANDLE
	*************************************************/
	virtual ALLOC_HANDLE_T Ptr2Handle(void* p)=0;


	virtual ALLOC_SIZE_T AskForDestroySize(ALLOC_HANDLE_T hHandl)=0;

    /*************************************************
	  Description:	���handle�Ƿ���Ч
	  Input:		�ڴ���
	  Output:		
      Return:	    0: ��Ч; -1:��Ч
	*************************************************/
    virtual int HandleIsValid (ALLOC_HANDLE_T mem_handle) = 0;
};


TTC_END_NAMESPACE

#endif
