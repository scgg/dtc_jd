#ifndef BIN_MALLOC_H
#define BIN_MALLOC_H

/************************************************************
  Description:   ��װ��ptmalloc�ڴ�����㷨�ĸ��ֽӿ�
  Version:         TTC 3.0
  Function List:   
    1.  Instance()		��ȡȫ�ֵ�CBinMalloc����
   2.  Destroy()		���ٶ���
   3. CheckBin() 		���bin�������Ƿ���ȷ
   4. Malloc()		�����ڴ�
   5. Calloc()		�����ڴ棬����ʼ��Ϊ0
   6. ReAlloc()		���·����ڴ�
   7. Free()			�ͷ��ڴ�
   8.ChunkSize()		��ȡ�ڴ��Ĵ�С
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

#define TTC_VER_MIN 4 // ��������ʶ��ttc�ڴ���С�汾

#define TTC_RESERVE_SIZE (4*1024UL)

#define EC_NO_MEM 2041 // �ڴ治�������
#define EC_KEY_EXIST 2042
#define EC_KEY_NOT_EXIST 2043
#define MAXSTATCOUNT 10000*3600*12

struct _CMemHead{
	uint32_t		m_auiSign[14]; // �ڴ��ʽ���
	unsigned short	m_ushVer; // �ڴ��ʽ�汾��
	unsigned short	m_ushHeadSize; // ͷ��С
	INTER_SIZE_T	m_tSize; // �ڴ��ܴ�С
	INTER_SIZE_T	m_tUserAllocSize; // �ϲ�Ӧ�÷��䵽���õ��ڴ��С
	INTER_SIZE_T    m_tUserAllocChunkCnt; // �ϲ�Ӧ�÷�����ڴ������
	uint32_t		m_uiFlags; // ���Ա��
	INTER_HANDLE_T	m_hBottom; // �ϲ�Ӧ�ÿ����ڴ�׵�ַ
	INTER_HANDLE_T	m_hReserveZone; // Ϊ�ϲ�Ӧ�ñ����ĵ�ַ
	INTER_HANDLE_T	m_hTop; // Ŀǰ���䵽����ߵ�ַ
	INTER_SIZE_T	m_tLastFreeChunkSize; // ���һ��free�󣬺ϲ��õ���chunk��С
	uint16_t		m_ushBinCnt; // bin������
	uint16_t		m_ushFastBinCnt; // fastbin����
	uint32_t		m_auiBinBitMap[(NBINS-1)/32+1]; // bin��bitmap
	uint32_t 		m_shmIntegrity;//�����ڴ������Ա��
	char			m_achReserv[872]; // �����ֶ� ��ʹCMemHead�Ĵ�СΪ1008Bytes�����Ϻ����bins��ﵽ4K��
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

    uint64_t statTmpDataSizeRecently;//���������ڴ��С
    uint64_t statTmpDataAllocCountRecently;//���������ڴ����
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

    //��С��chrunk size,
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

	// �ڲ���һ��ͳ��
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
	  Description:	��ʽ���ڴ�
	  Input:		pAddr	�ڴ���ַ
				tSize		�ڴ���С
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Init(void* pAddr, INTER_SIZE_T tSize);
	
	/*************************************************
	  Description:	attach�Ѿ���ʽ���õ��ڴ��
	  Input:		pAddr	�ڴ���ַ
				tSize		�ڴ���С
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Attach(void* pAddr, INTER_SIZE_T tSize);
	
	/*************************************************
	  Description:	����ڴ���ttc�汾
	  Input:		pAddr	�ڴ���ַ
				tSize		�ڴ���С
	   Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int DetectVersion();

	/* �����ڴ������Լ��ӿ� */
	int  ShareMemoryIntegrity();
	void SetShareMemoryIntegrity(const int flag);
	
	/*************************************************
	  Description:	����ڲ����ݽṹbin�Ƿ���ȷ
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int CheckBin();
#if	BIN_MEM_CHECK
	int CheckMem();
#endif
	int DumpBins();
	int DumpMem();
	
	/*************************************************
	  Description:	�����ڴ�
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	ALLOC_HANDLE_T Malloc(ALLOC_SIZE_T tSize);

	/*************************************************
	  Description:	�����ڴ棬�����ڴ��ʼ��Ϊ0
	  Input:		tSize		������ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��
	*************************************************/
	ALLOC_HANDLE_T Calloc(ALLOC_SIZE_T tSize);

	/*************************************************
	  Description:	���·����ڴ�
	  Input:		hHandle	���ڴ���
				tSize		�·�����ڴ��С
	  Output:		
	  Return:		�ڴ������INVALID_HANDLEΪʧ��(ʧ��ʱ�����ͷ����ڴ��)
	*************************************************/
	ALLOC_HANDLE_T ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize);
	
	/*************************************************
	  Description:	�ͷ��ڴ�
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Free(ALLOC_HANDLE_T hHandle);

	/*************************************************
	  Description: ��ȡ�ͷ�����ڴ����Եõ�����free�ռ�	
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		>0Ϊ�ɹ���0ʧ��
	*************************************************/
	unsigned AskForDestroySize(ALLOC_HANDLE_T hHandle);

	/*************************************************
	  Description:	��ȡ�ڴ���С
	  Input:		hHandle	�ڴ���
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	ALLOC_SIZE_T ChunkSize(ALLOC_HANDLE_T hHandle);
	
	/*************************************************
	  Description:	��ȡ�û��Ѿ�������ڴ��ܴ�С
	  Input:		
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	INTER_SIZE_T UserAllocSize(){ return m_pstHead->m_tUserAllocSize; }
	
	/*************************************************
	  Description:	��ȡ�ڴ��ܴ�С
	  Input:		
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	INTER_SIZE_T TotalSize(){ return m_pstHead->m_tSize; }
	
	/*************************************************
	  Description:	���һ���ͷ��ڴ棬�ϲ����chunk��С
	  Input:		
	  Output:		
	  Return:		�ڴ��С
	*************************************************/
	ALLOC_SIZE_T LastFreeSize();
	
	/*************************************************
	  Description:	��ȡΪ�ϲ�Ӧ�ñ������ڴ�飨��СΪTTC_RESERVE_SIZE��4K��
	  Input:		
	  Output:		
	  Return:		�ڴ���
	*************************************************/
	ALLOC_HANDLE_T  GetReserveZone();


	/*************************************************
	  Description:	�����ת�����ڴ��ַ
	  Input:		�ڴ���
	  Output:		
	  Return:		�ڴ��ַ����������Ч����NULL
	*************************************************/
	inline void* Handle2Ptr(ALLOC_HANDLE_T hHandle){ 
	    if(hHandle == INVALID_HANDLE)
		return(NULL);
	    return (void*)(((char*)m_pBaseAddr)+hHandle);
	}
	
	/*************************************************
	  Description:	���ڴ��ַת��Ϊ���
	  Input:		�ڴ��ַ
	  Output:		
	  Return:		�ڴ����������ַ��Ч����INVALID_HANDLE
	*************************************************/
	inline ALLOC_HANDLE_T Ptr2Handle(void* p){ 
	    if((char*)p < (char*)m_pBaseAddr || (char*)p >= ((char*)m_pBaseAddr)+m_pstHead->m_tSize)
		return INVALID_HANDLE;
	    return (ALLOC_HANDLE_T)(((char*)p) - ((char*)m_pBaseAddr)); 
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


TTC_END_NAMESPACE

#endif
