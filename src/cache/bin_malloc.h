#ifndef BIN_MALLOC_H
#define BIN_MALLOC_H

/************************************************************
  Description:   封装了ptmalloc内存分配算法的各种接口
  Version:         TTC 3.0
  Function List:   
    1.  Instance()		获取全局的CBinMalloc对象
   2.  Destroy()		销毁对象
   3. CheckBin() 		检查bin的数据是否正确
   4. Malloc()		分配内存
   5. Calloc()		分配内存，并初始化为0
   6. ReAlloc()		重新分配内存
   7. Free()			释放内存
   8.ChunkSize()		获取内存块的大小
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include <stdint.h>
#include <stdlib.h>
#include "namespace.h"
#include "mallocator.h"
#include "log.h"
#include "StatTTC.h"

TTC_BEGIN_NAMESPACE


#define MALLOC_FLAG_FAST 0x1

/*
  This struct declaration is misleading (but accurate and necessary).
  It declares a "view" into memory allowing access to necessary
  fields at known offsets from a given base. See explanation below.
*/

typedef struct{
  ALLOC_SIZE_T	m_tPreSize;  /* Size of previous chunk (if free).  */
  ALLOC_SIZE_T	m_tSize;       /* Size in bytes, including overhead. */

  INTER_HANDLE_T	m_hPreChunk;         /* double links -- used only if free. */
  INTER_HANDLE_T	m_hNextChunk;
}CMallocChunk;

typedef struct{
	INTER_HANDLE_T	m_hPreChunk;         
	INTER_HANDLE_T	m_hNextChunk;
}CBin;

/* The smallest possible chunk */
#define MIN_CHUNK_SIZE        (sizeof(CMallocChunk))

/* The smallest size we can malloc is an aligned minimal chunk */
#define MINSIZE (unsigned long)(((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK))

#define NBINS             128
#define NSMALLBINS         64
#define SMALLBIN_WIDTH      8
#define MIN_LARGE_SIZE    512

#define TTC_SIGN_0 0
#define TTC_SIGN_1 0x4D635474U
#define TTC_SIGN_2 1
#define TTC_SIGN_3 0xFFFFFFFFU
#define TTC_SIGN_4 0xFFFFFFFFU
#define TTC_SIGN_5 0xFFFFFFFFU
#define TTC_SIGN_6 4
#define TTC_SIGN_7 0
#define TTC_SIGN_8 16
#define TTC_SIGN_9 0xFFFFFFFFU
#define TTC_SIGN_A 0
#define TTC_SIGN_B 0
#define TTC_SIGN_C 0xFFFFFFFFU
#define TTC_SIGN_D 0xFFFFFFFFU

#define TTC_VER_MIN 4 // 本代码认识的ttc内存最小版本

#define TTC_RESERVE_SIZE (4*1024UL)

#define EC_NO_MEM 2041 // 内存不足错误码
#define EC_KEY_EXIST 2042
#define EC_KEY_NOT_EXIST 2043
#define MAXSTATCOUNT 10000*3600*12

struct _CMemHead{
	uint32_t		m_auiSign[14]; // 内存格式标记
	unsigned short	m_ushVer; // 内存格式版本号
	unsigned short	m_ushHeadSize; // 头大小
	INTER_SIZE_T	m_tSize; // 内存总大小
	INTER_SIZE_T	m_tUserAllocSize; // 上层应用分配到可用的内存大小
	INTER_SIZE_T    m_tUserAllocChunkCnt; // 上层应用分配的内存块数量
	uint32_t		m_uiFlags; // 特性标记
	INTER_HANDLE_T	m_hBottom; // 上层应用可用内存底地址
	INTER_HANDLE_T	m_hReserveZone; // 为上层应用保留的地址
	INTER_HANDLE_T	m_hTop; // 目前分配到的最高地址
	INTER_SIZE_T	m_tLastFreeChunkSize; // 最近一次free后，合并得到的chunk大小
	uint16_t		m_ushBinCnt; // bin的数量
	uint16_t		m_ushFastBinCnt; // fastbin数量
	uint32_t		m_auiBinBitMap[(NBINS-1)/32+1]; // bin的bitmap
	uint32_t 		m_shmIntegrity;//共享内存完整性标记
	char			m_achReserv[872]; // 保留字段 （使CMemHead的大小为1008Bytes，加上后面的bins后达到4K）
}__attribute__((__aligned__(4)));
typedef struct _CMemHead CMemHead;

#define GET_OBJ(mallocter,handle,obj_ptr) do{ 				\
		obj_ptr = (typeof(obj_ptr))mallocter.Handle2Ptr(handle); \
}while(0)


class CBinMalloc: public CMallocator
{
private:
	void*	m_pBaseAddr;
	CMemHead*	m_pstHead;
	CBin*	m_ptBin;
	CBin*	m_ptFastBin;
	CBin*	m_ptUnsortedBin;
	char	m_szErr[200];

	// stat
	CStatItemU32 statChunkTotal;
	CStatItem statDataSize;
	CStatItem statMemoryTop;

    uint64_t statTmpDataSizeRecently;//最近分配的内存大小
    uint64_t statTmpDataAllocCountRecently;//最近分配的内存次数
    CStatItem statAverageDataSizeRecently;
    inline void AddAllocSizeToStat(uint64_t size)
    {
        if (statTmpDataAllocCountRecently > MAXSTATCOUNT)
        {
            statTmpDataSizeRecently = 0;
            statTmpDataAllocCountRecently = 0;
            statAverageDataSizeRecently = MINSIZE;
        }
        else
        {
        statTmpDataSizeRecently += size;
        statTmpDataAllocCountRecently++;
        statAverageDataSizeRecently = statTmpDataSizeRecently/statTmpDataAllocCountRecently;
        }
    }

    //最小的chrunk size,
    unsigned int minChunkSize;
    inline unsigned int GetMinChunkSize(void)
    {
       return minChunkSize==1?
           (
            (statChunkTotal <=0)?
            MINSIZE:
            statDataSize/statChunkTotal
            ):
           minChunkSize;
    }
public:
    void SetMinChunkSize(unsigned int size)
    {
        minChunkSize = size == 1?  1: (size<MINSIZE?MINSIZE:size);
    }

	
protected:
	void InitSign();
	
	void* BinMalloc(CBin& ptBin);
	void* SmallBinMalloc(ALLOC_SIZE_T tSize);
	void* FastMalloc(ALLOC_SIZE_T tSize);
	void* TopAlloc(ALLOC_SIZE_T tSize);
	int UnlinkBin(CBin& stBin, INTER_HANDLE_T hHandle);
	int LinkBin(CBin& stBin, INTER_HANDLE_T hHandle);
	int LinkSortedBin(CBin& stBin, INTER_HANDLE_T hHandle, ALLOC_SIZE_T tSize);
	int CheckInuseChunk(CMallocChunk* pstChunk);
	int FreeFast();

	inline void SetBinBitMap(unsigned int uiBinIdx){
		m_pstHead->m_auiBinBitMap[uiBinIdx/32] |= (1UL<<(uiBinIdx%32));
	}
	inline void ClearBinBitMap(unsigned int uiBinIdx){
		m_pstHead->m_auiBinBitMap[uiBinIdx/32] &= (~(1UL<<(uiBinIdx%32)));
	}
	inline int EmptyBin(unsigned int uiBinIdx){
		return (m_ptBin[uiBinIdx].m_hNextChunk == INVALID_HANDLE);
	}

	// 内部做一下统计
	ALLOC_HANDLE_T InterMalloc(ALLOC_SIZE_T tSize);
	ALLOC_HANDLE_T InterReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize, ALLOC_SIZE_T& tOldMemSize);
	int InterFree(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T& tMemSize);

public:
	CBinMalloc();
	~CBinMalloc();
	
	static CBinMalloc *Instance();
	static void Destroy();

	template <class T>
	    T * Pointer(ALLOC_HANDLE_T hHandle){return reinterpret_cast<T *>(Handle2Ptr(hHandle));}

	ALLOC_HANDLE_T Handle(void *p) {return Ptr2Handle(p);}

	const char* GetErrMsg(){ return m_szErr; }
	const CMemHead* GetHeadInfo() const { return m_pstHead; }
	
	/*************************************************
	  Description:	格式化内存
	  Input:		pAddr	内存块地址
				tSize		内存块大小
	  Return:		0为成功，非0失败
	*************************************************/
	int Init(void* pAddr, INTER_SIZE_T tSize);
	
	/*************************************************
	  Description:	attach已经格式化好的内存块
	  Input:		pAddr	内存块地址
				tSize		内存块大小
	  Return:		0为成功，非0失败
	*************************************************/
	int Attach(void* pAddr, INTER_SIZE_T tSize);
	
	/*************************************************
	  Description:	检测内存块的ttc版本
	  Input:		pAddr	内存块地址
				tSize		内存块大小
	   Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int DetectVersion();

	/* 共享内存完整性检测接口 */
	int  ShareMemoryIntegrity();
	void SetShareMemoryIntegrity(const int flag);
	
	/*************************************************
	  Description:	检测内部数据结构bin是否正确
	  Input:		
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int CheckBin();
#if	BIN_MEM_CHECK
	int CheckMem();
#endif
	int DumpBins();
	int DumpMem();
	
	/*************************************************
	  Description:	分配内存
	  Input:		tSize		分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败
	*************************************************/
	ALLOC_HANDLE_T Malloc(ALLOC_SIZE_T tSize);

	/*************************************************
	  Description:	分配内存，并将内存初始化为0
	  Input:		tSize		分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败
	*************************************************/
	ALLOC_HANDLE_T Calloc(ALLOC_SIZE_T tSize);

	/*************************************************
	  Description:	重新分配内存
	  Input:		hHandle	老内存句柄
				tSize		新分配的内存大小
	  Output:		
	  Return:		内存块句柄，INVALID_HANDLE为失败(失败时不会释放老内存块)
	*************************************************/
	ALLOC_HANDLE_T ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize);
	
	/*************************************************
	  Description:	释放内存
	  Input:		hHandle	内存句柄
	  Output:		
	  Return:		0为成功，非0失败
	*************************************************/
	int Free(ALLOC_HANDLE_T hHandle);

	/*************************************************
	  Description: 获取释放这块内存后可以得到多少free空间	
	  Input:		hHandle	内存句柄
	  Output:		
	  Return:		>0为成功，0失败
	*************************************************/
	unsigned AskForDestroySize(ALLOC_HANDLE_T hHandle);

	/*************************************************
	  Description:	获取内存块大小
	  Input:		hHandle	内存句柄
	  Output:		
	  Return:		内存大小
	*************************************************/
	ALLOC_SIZE_T ChunkSize(ALLOC_HANDLE_T hHandle);
	
	/*************************************************
	  Description:	获取用户已经分配的内存总大小
	  Input:		
	  Output:		
	  Return:		内存大小
	*************************************************/
	INTER_SIZE_T UserAllocSize(){ return m_pstHead->m_tUserAllocSize; }
	
	/*************************************************
	  Description:	获取内存总大小
	  Input:		
	  Output:		
	  Return:		内存大小
	*************************************************/
	INTER_SIZE_T TotalSize(){ return m_pstHead->m_tSize; }
	
	/*************************************************
	  Description:	最近一次释放内存，合并后的chunk大小
	  Input:		
	  Output:		
	  Return:		内存大小
	*************************************************/
	ALLOC_SIZE_T LastFreeSize();
	
	/*************************************************
	  Description:	获取为上层应用保留的内存块（大小为TTC_RESERVE_SIZE＝4K）
	  Input:		
	  Output:		
	  Return:		内存句柄
	*************************************************/
	ALLOC_HANDLE_T  GetReserveZone();


	/*************************************************
	  Description:	将句柄转换成内存地址
	  Input:		内存句柄
	  Output:		
	  Return:		内存地址，如果句柄无效返回NULL
	*************************************************/
	inline void* Handle2Ptr(ALLOC_HANDLE_T hHandle){ 
	    if(hHandle == INVALID_HANDLE)
		return(NULL);
	    return (void*)(((char*)m_pBaseAddr)+hHandle);
	}
	
	/*************************************************
	  Description:	将内存地址转换为句柄
	  Input:		内存地址
	  Output:		
	  Return:		内存句柄，如果地址无效返回INVALID_HANDLE
	*************************************************/
	inline ALLOC_HANDLE_T Ptr2Handle(void* p){ 
	    if((char*)p < (char*)m_pBaseAddr || (char*)p >= ((char*)m_pBaseAddr)+m_pstHead->m_tSize)
		return INVALID_HANDLE;
	    return (ALLOC_HANDLE_T)(((char*)p) - ((char*)m_pBaseAddr)); 
	}

    /*************************************************
	  Description:	检测handle是否有效
	  Input:		内存句柄
	  Output:		
      Return:	    0: 有效; -1:无效
	*************************************************/
    virtual int HandleIsValid (ALLOC_HANDLE_T mem_handle)
    {
        return 0;
    }
};


TTC_END_NAMESPACE

#endif
