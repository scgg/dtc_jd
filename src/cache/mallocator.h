#ifndef MALLOCATOR_H
#define MALLOCATOR_H

/************************************************************
  Description:   定义了内存分配器的各种接口(纯抽象类)
  Version:         TTC 3.0
  Function List:   
   1. Malloc()		分配内存
   2. Calloc()		分配内存，并初始化为0
   3. ReAlloc()		重新分配内存
   4. Free()			释放内存
   5.ChunkSize()		获取内存块的大小
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
	  Description:	分配内存
	  Input:		tSize		分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败
	*************************************************/
	virtual ALLOC_HANDLE_T Malloc(ALLOC_SIZE_T tSize)=0;

	/*************************************************
	  Description:	分配内存，并将内存初始化为0
	  Input:		tSize		分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败
	*************************************************/
	virtual ALLOC_HANDLE_T Calloc(ALLOC_SIZE_T tSize)=0;

	/*************************************************
	  Description:	重新分配内存
	  Input:		hHandle	老内存句柄
				tSize		新分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败(失败时不会释放老内存块)
	*************************************************/
	virtual ALLOC_HANDLE_T ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize)=0;
	
	/*************************************************
	  Description:	释放内存
	  Input:		hHandle	内存句柄
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	virtual int Free(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	获取内存块大小
	  Input:		hHandle	内存句柄
	  Output:		
	  Return:		内存大小
	*************************************************/
	virtual ALLOC_SIZE_T ChunkSize(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	将句柄转换成内存地址
	  Input:		内存句柄
	  Output:		
	  Return:		内存地址，如果句柄无效返回NULL
	*************************************************/
	virtual void* Handle2Ptr(ALLOC_HANDLE_T hHandle)=0;
	
	/*************************************************
	  Description:	将内存地址转换为句柄
	  Input:		内存地址
	  Output:		
	  Return:		内存句柄，如果地址无效返回INVALID_HANDLE
	*************************************************/
	virtual ALLOC_HANDLE_T Ptr2Handle(void* p)=0;


	virtual ALLOC_SIZE_T AskForDestroySize(ALLOC_HANDLE_T hHandl)=0;

    /*************************************************
	  Description:	检测handle是否有效
	  Input:		内存句柄
	  Output:		
      Return:	    0: 有效; -1:无效
	*************************************************/
    virtual int HandleIsValid (ALLOC_HANDLE_T mem_handle) = 0;
};


TTC_END_NAMESPACE

#endif
