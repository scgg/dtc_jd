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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "log.h"
#include "bin_malloc.h"
#include "singleton.h"

TTC_USING_NAMESPACE

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(h)   (void*)(((char*)h) + 2*sizeof(ALLOC_SIZE_T))
#define mem2chunk(h) (void*)(((char*)h) - 2*sizeof(ALLOC_SIZE_T))
#define chunkhandle2memhandle(handle) (handle + 2*sizeof(ALLOC_SIZE_T))
#define memhandle2chunkhandle(handle) (handle - 2*sizeof(ALLOC_SIZE_T))
#if BIN_MEM_CHECK
#define chunksize2memsize(size) (size - 2*sizeof(ALLOC_SIZE_T))
#define checked_chunksize2memsize(size) (size>2*sizeof(ALLOC_SIZE_T)?(size - 2*sizeof(ALLOC_SIZE_T)):0)
#else
#define chunksize2memsize(size) (size - sizeof(ALLOC_SIZE_T))
#define checked_chunksize2memsize(size) (size>sizeof(ALLOC_SIZE_T)?(size - sizeof(ALLOC_SIZE_T)):0)
#endif

/* Check if m has acceptable alignment */

#define aligned_OK(m)  (((unsigned long)(m) & MALLOC_ALIGN_MASK) == 0)

#define misaligned_chunk(h) \
  ( (MALLOC_ALIGNMENT == 2 * SIZE_SZ ? (h) : chunkhandle2memhandle(h)) & MALLOC_ALIGN_MASK)

/*
   Check if a request is so large that it would wrap around zero when
   padded and aligned. To simplify some other code, the bound is made
   low enough so that adding MINSIZE will also not wrap around zero.
*/

#define REQUEST_OUT_OF_RANGE(req)                                 \
  ((unsigned long)(req) >=                                        \
   (unsigned long)(ALLOC_SIZE_T)(-2 * MINSIZE))

/* pad request bytes into a usable size -- internal version */
//#if BIN_MEM_CHECK
#define request2size(req)                                         \
  (((req) + 2*SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?           \
   MINSIZE :                                                      \
   ((req) + 2*SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
/*
#else
#define request2size(req)                                         \
  (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?             \
   MINSIZE :                                                      \
   ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)
#endif
*/

/*  Same, except also perform argument check */

#define checked_request2size(req, sz)                             \
  if (REQUEST_OUT_OF_RANGE(req)) {                                \
    return(INVALID_HANDLE);                                       \
  }                                                               \
  (sz) = request2size(req);

/*
  --------------- Physical chunk operations ---------------
*/
/* size field is or'ed with PREV_INUSE when previous adjacent chunk in use */
#define PREV_INUSE 0x1
#define RESERVE_BITS (0x2|0x4)
/*
  Bits to mask off when extracting size
*/
#define SIZE_BITS (PREV_INUSE|RESERVE_BITS)

/* Get size, ignoring use bits */
#define CHUNK_SIZE(p)         ((p)->m_tSize & ~(SIZE_BITS))
#define REAL_SIZE(sz)		((sz) & ~(SIZE_BITS))


/* extract inuse bit of previous chunk */
#define prev_inuse(p)       ((p)->m_tSize & PREV_INUSE)
#define inuse_bit_at_offset(p,offset) (((CMallocChunk*)(((char*)p)+offset))->m_tSize & PREV_INUSE)
#define set_inuse_bit_at_offset(p, s) (((CMallocChunk*)(((char*)(p)) + (s)))->m_tSize |= PREV_INUSE)
#define clear_inuse_bit_at_offset(p, s) (((CMallocChunk*)(((char*)(p)) + (s)))->m_tSize &= ~(PREV_INUSE))
#define set_size_at_offset(p, offset, size) (((CMallocChunk*)(((char*)p)+(offset)))->m_tSize = REAL_SIZE(size) | (((CMallocChunk*)(((char*)p)+(offset)))->m_tSize & SIZE_BITS)) 
#define set_presize_at_offset(p, offset, size) (((CMallocChunk*)(((char*)p)+(offset)))->m_tPreSize = REAL_SIZE(size)) 



#define in_smallbin_range(sz)  \
  ((unsigned long)(sz) < (unsigned long)MIN_LARGE_SIZE)

#define smallbin_index(sz)     (((unsigned)(sz)) >> 3)

#define largebin_index(sz)                                                   \
(((((unsigned long)(sz)) >>  6) <= 32)?  56 + (((unsigned long)(sz)) >>  6): \
 ((((unsigned long)(sz)) >>  9) <= 20)?  91 + (((unsigned long)(sz)) >>  9): \
 ((((unsigned long)(sz)) >> 12) <= 10)? 110 + (((unsigned long)(sz)) >> 12): \
 ((((unsigned long)(sz)) >> 15) <=  4)? 119 + (((unsigned long)(sz)) >> 15): \
 ((((unsigned long)(sz)) >> 18) <=  2)? 124 + (((unsigned long)(sz)) >> 18): \
                                        126)

#define bin_index(sz) \
 ((in_smallbin_range(sz)) ? smallbin_index(sz) : largebin_index(sz))

#define NFASTBINS	NSMALLBINS
#define FAST_MAX_SIZE MIN_LARGE_SIZE
#define fastbin_index(sz) smallbin_index(sz)

#define AT_TOP(chunk,sz) (((char*)chunk)+sz == ((char*)m_pBaseAddr)+m_pstHead->m_hTop)

#define CAN_COMBILE(size, add) ((INTER_SIZE_T)size+add <= (INTER_SIZE_T)MAX_ALLOC_SIZE)

CBinMalloc::CBinMalloc()
{
	m_pBaseAddr = NULL;
	m_pstHead = NULL;
	m_ptBin = NULL;
	m_ptFastBin = NULL;
	m_ptUnsortedBin = NULL;
	statChunkTotal	= statmgr.GetItemU32(TTC_CHUNK_TOTAL);
	statDataSize	= statmgr.GetItem(TTC_DATA_SIZE);
	statMemoryTop	= statmgr.GetItem(TTC_MEMORY_TOP);
    statTmpDataSizeRecently = 0;
    statTmpDataAllocCountRecently = 0;
    statAverageDataSizeRecently =  statmgr.GetItem(DATA_SIZE_AVG_RECENT);
	memset(m_szErr, 0, sizeof(m_szErr));
    minChunkSize = MINSIZE;
}

CBinMalloc::~CBinMalloc()
{
}

CBinMalloc *CBinMalloc::Instance()
{
    return CSingleton<CBinMalloc>::Instance();
}

void CBinMalloc::Destroy()
{
   CSingleton<CBinMalloc>::Destroy();
}
/*��ʼ��header�е�signature��*/
void CBinMalloc::InitSign()
{
	static const unsigned int V4Sign[14] = {
		TTC_SIGN_0,
		TTC_SIGN_1,
		TTC_SIGN_2,
		TTC_SIGN_3,
		TTC_SIGN_4,
		TTC_SIGN_5,
		TTC_SIGN_6,
		TTC_SIGN_7,
		TTC_SIGN_8,
		TTC_SIGN_9,
		TTC_SIGN_A,
		TTC_SIGN_B,
		TTC_SIGN_C,
		TTC_SIGN_D
	};

	memcpy(m_pstHead->m_auiSign, V4Sign, sizeof(m_pstHead->m_auiSign));
}

#if __WORDSIZE == 64
# define UINT64FMT_T "%lu"
#else
# define UINT64FMT_T "%llu"
#endif
/*��ʼ��cacheͷ��Ϣ*/
/*���������cache����ʼ��ַ��cache���ܴ�С*/
/*ͷ��Ϣ�������μ�newmanwang��TTC�ڴ������ʵ�ֵķ���*/
int CBinMalloc::Init(void* pAddr, INTER_SIZE_T tSize)
{
	int i;
	
	if(tSize < sizeof(CMemHead) + sizeof(CBin)*(NBINS+NFASTBINS+1) + TTC_RESERVE_SIZE + MINSIZE){
		snprintf(m_szErr, sizeof(m_szErr), "invalid size[" UINT64FMT_T "]", tSize);
		return(-1);
	}
	
	m_pBaseAddr = pAddr;
	m_pstHead = (CMemHead*)m_pBaseAddr;
	memset(m_pstHead, 0, sizeof(CMemHead));
	InitSign();
	m_pstHead->m_ushVer = TTC_VER_MIN;
	m_pstHead->m_ushHeadSize = sizeof(CMemHead);
	m_pstHead->m_tSize = tSize;
	m_pstHead->m_tUserAllocChunkCnt = 0;
	m_pstHead->m_hReserveZone = sizeof(CMemHead) + sizeof(CBin)*(NBINS+NFASTBINS+1);
	m_pstHead->m_hReserveZone = (m_pstHead->m_hReserveZone + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK;
	m_pstHead->m_hBottom = (m_pstHead->m_hReserveZone + TTC_RESERVE_SIZE + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK;
	m_pstHead->m_hTop = m_pstHead->m_hBottom;
	m_pstHead->m_tUserAllocSize = m_pstHead->m_hBottom;
	statMemoryTop = m_pstHead->m_hTop;
	m_pstHead->m_tLastFreeChunkSize = (tSize > m_pstHead->m_hTop + MINSIZE)?(tSize - m_pstHead->m_hTop - MINSIZE):0;
	m_pstHead->m_ushBinCnt = NBINS;
	m_pstHead->m_ushFastBinCnt = NFASTBINS;
	memset(m_pstHead->m_auiBinBitMap, 0, sizeof(m_pstHead->m_auiBinBitMap));
	m_ptBin = (CBin*)(((char*)m_pBaseAddr)+sizeof(CMemHead));
	m_ptFastBin = m_ptBin+NBINS;
	m_ptUnsortedBin = m_ptFastBin+NFASTBINS;
	
	for(i=0; i<NBINS; i++){
		m_ptBin[i].m_hPreChunk = INVALID_HANDLE;
		m_ptBin[i].m_hNextChunk = INVALID_HANDLE;
	}
	
	for(i=0; i<NFASTBINS; i++){
		m_ptFastBin[i].m_hPreChunk = INVALID_HANDLE;
		m_ptFastBin[i].m_hNextChunk = INVALID_HANDLE;
	}
	
	m_ptUnsortedBin[0].m_hPreChunk = INVALID_HANDLE;
	m_ptUnsortedBin[0].m_hNextChunk = INVALID_HANDLE;
	
	CMallocChunk* pstChunk;
	pstChunk = (CMallocChunk*)Handle2Ptr(m_pstHead->m_hTop);
	pstChunk->m_tPreSize = 0;
	pstChunk->m_tSize = PREV_INUSE;
	
	// init stat
	statChunkTotal = m_pstHead->m_tUserAllocChunkCnt;
	statDataSize = m_pstHead->m_tUserAllocSize;
	
	return(0);
}
/*У��cache�İ汾�Ƿ���ȷ*/
int CBinMalloc::DetectVersion()
{
	if(m_pstHead->m_auiSign[0] != TTC_SIGN_0 || m_pstHead->m_auiSign[1] != TTC_SIGN_1)
		return 1;
	if(m_pstHead->m_ushVer == 2)
		return(2);
	if(m_pstHead->m_ushVer == 3)
		return(3);
	if(m_pstHead->m_ushVer == 4)
		return(4);
	
	snprintf(m_szErr, sizeof(m_szErr), "unknown version signature %u", m_pstHead->m_ushVer);
	return(0);
}
/*�鿴cache�Ƿ�һ�£�������ttc������cache��ʱ��ֻҪ����Ҫдcache���ͻ����ò�һ�£���ֹttc������ʱcrash�������󲻾����ʹ���ҵ����ڴ�*/
int CBinMalloc::ShareMemoryIntegrity()
{
	return (int)m_pstHead->m_shmIntegrity;
}

void CBinMalloc::SetShareMemoryIntegrity(const int flags)
{
	if(flags)
		m_pstHead->m_shmIntegrity = 1;
	else
		m_pstHead->m_shmIntegrity = 0;
}
/*�����Ѿ����ڵ�IPC shared memory��ttc��������Ὣ������ڴ���Ϊcache�������������cache��ͷ��Ϣ���Ƿ���ȷ*/
int CBinMalloc::Attach(void* pAddr, INTER_SIZE_T tSize)
{

	if(tSize < sizeof(CMemHead) + sizeof(CBin)*(NBINS+NFASTBINS+1) + MINSIZE){
		snprintf(m_szErr, sizeof(m_szErr), "invalid size[" UINT64FMT_T "]", tSize);
		return(-1);
	}
	
	m_pBaseAddr = pAddr;
	m_pstHead = (CMemHead*)m_pBaseAddr;
	if(DetectVersion() != TTC_VER_MIN){
		snprintf(m_szErr, sizeof(m_szErr), "Unsupported preferred version %u", m_pstHead->m_ushVer);
		return(-2);
	}
	
	if(m_pstHead->m_tSize != tSize){
		snprintf(m_szErr, sizeof(m_szErr), "invalid argument");
		return(-3);
	}
	if(m_pstHead->m_hTop >= m_pstHead->m_tSize){
		snprintf(m_szErr, sizeof(m_szErr), "memory corruption-invalid bottom value");
		return(-4);
	}
	m_ptBin = (CBin*)(((char*)m_pBaseAddr)+sizeof(CMemHead));
	m_ptFastBin = m_ptBin+NBINS;
	m_ptUnsortedBin = m_ptFastBin+NFASTBINS;
	
	// init stat
	statChunkTotal = m_pstHead->m_tUserAllocChunkCnt;
	statDataSize = m_pstHead->m_tUserAllocSize;
	
	return(0);
}

ALLOC_HANDLE_T CBinMalloc::GetReserveZone()
{
    return m_pstHead->m_hReserveZone;
}
/*���������chunk���û�handle*/
/*�������chunk���û�ʹ�ÿռ�Ĵ�С*/
ALLOC_SIZE_T CBinMalloc::ChunkSize(ALLOC_HANDLE_T hHandle)
{
	CMallocChunk* pstChunk;
	
	if(hHandle >= m_pstHead->m_hTop || hHandle <= m_pstHead->m_hBottom){
		snprintf(m_szErr, sizeof(m_szErr), "[ChunkSize]-invalid handle");
		return(0);
	}
	 
	pstChunk = (CMallocChunk*)mem2chunk(Handle2Ptr(hHandle));
	
	if(CheckInuseChunk(pstChunk) != 0){
		snprintf(m_szErr, sizeof(m_szErr), "[ChunkSize]-invalid chunk");
		return(0);
	}
	
	return chunksize2memsize(CHUNK_SIZE(pstChunk));
}
/*��������bin�ϵ�ͷchunkΪʹ��״̬���������chunk��bin������*/
void* CBinMalloc::BinMalloc(CBin& ptBin)
{
	CMallocChunk* pstChunk;
	void* p;
	
	if(ptBin.m_hNextChunk == INVALID_HANDLE)
		return(NULL);
	
	p = Handle2Ptr(ptBin.m_hNextChunk);
	pstChunk = (CMallocChunk*)p;
	set_inuse_bit_at_offset(pstChunk, REAL_SIZE(pstChunk->m_tSize));
	UnlinkBin(ptBin, ptBin.m_hNextChunk);
	
	return p;	
}
/*�����е�bin��飺small&large bins, fast bins, unsorted bins*/
/*У�鷽����ÿ��bin���һ��˫���ѭ������*/
int CBinMalloc::CheckBin()
{
	int i;
	
	INTER_HANDLE_T hHandle;
	CMallocChunk* pstChunk;
	for(i=0; i<NBINS; i++){
		hHandle = m_ptBin[i].m_hNextChunk;
		if(hHandle != INVALID_HANDLE){
			do{
				pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
				if(pstChunk->m_hNextChunk != INVALID_HANDLE)
					hHandle = pstChunk->m_hNextChunk;
			}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
		}
		if(m_ptBin[i].m_hPreChunk != hHandle){
			snprintf(m_szErr, sizeof(m_szErr), "bad bin[%d]", i);
			return(-1);
		}
	}

	for(i=0; i<NFASTBINS; i++){
		hHandle = m_ptFastBin[i].m_hNextChunk;
		if(hHandle != INVALID_HANDLE){
			do{
				pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
				if(pstChunk->m_hNextChunk != INVALID_HANDLE)
					hHandle = pstChunk->m_hNextChunk;
			}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
		}
		if(m_ptFastBin[i].m_hPreChunk != hHandle){
			snprintf(m_szErr, sizeof(m_szErr), "bad fast-bin[%d]", i);
			return(-2);
		}	
	}

	hHandle = m_ptUnsortedBin[0].m_hNextChunk;
	if(hHandle != INVALID_HANDLE){
		do{
			pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
			if(pstChunk->m_hNextChunk != INVALID_HANDLE)
				hHandle = pstChunk->m_hNextChunk;
		}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
	}
	if(m_ptUnsortedBin[0].m_hPreChunk != hHandle){
#if __WORDSIZE == 64
		snprintf(m_szErr, sizeof(m_szErr), "bad unsorted-bin[%d] %lu!=%lu", 0, m_ptUnsortedBin[0].m_hPreChunk, hHandle);
#else
		snprintf(m_szErr, sizeof(m_szErr), "bad unsorted-bin[%d] %llu!=%llu", 0, m_ptUnsortedBin[0].m_hPreChunk, hHandle);
#endif
		return(-3);
	}	

	return(0);
}
/*У������bin�е�chunk��һ����*/
/*���鷽�����ӷ����top�߿�ʼ��bottom����һ��chunkһ��chunk�ļ�飬������chunk�Ĵ�С�ǲ��Ǻ����ĺ�һ��chunk��presizeһ��*/
#if BIN_MEM_CHECK
int CBinMalloc::CheckMem()
{	
	INTER_HANDLE_T hHandle;
	CMallocChunk* pstChunk;
	ALLOC_SIZE_T tSize;

	tSize = 0;
	hHandle = m_pstHead->m_hTop;
	while(hHandle > m_pstHead->m_hBottom){
		pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
		if(CHUNK_SIZE(pstChunk) != tSize){
#if __WORDSIZE == 64		
			snprintf(m_szErr, sizeof(m_szErr), "bad memory1 handle[%lu]", hHandle);
#else
			snprintf(m_szErr, sizeof(m_szErr), "bad memory1 handle[%llu]", hHandle);
#endif
			return(-1);
		}
		tSize = pstChunk->m_tPreSize;
		if(hHandle < tSize){
#if __WORDSIZE == 64		
			snprintf(m_szErr, sizeof(m_szErr), "bad memory handle[%lu]", hHandle);
#else
			snprintf(m_szErr, sizeof(m_szErr), "bad memory handle[%llu]", hHandle);
#endif
			return(-2);
		}
		hHandle -= tSize;
	}

	return(0);
}
#endif
/*��fastbins��һ��bin��ȡһ������chunk,����tsize��С��*/
/*bin���������ҷ����ǣ�������smallbins�в���bin�ķ�������*/
void* CBinMalloc::FastMalloc(ALLOC_SIZE_T tSize)
{
	return BinMalloc(m_ptFastBin[smallbin_index(tSize)]);
}
/*��smallbins��һ��bin��ȡһ������chunk����tsize��С*/
void* CBinMalloc::SmallBinMalloc(ALLOC_SIZE_T tSize)
{
	void* p;
	unsigned int uiBinIdx;
	
	uiBinIdx = smallbin_index(tSize);
	p = BinMalloc(m_ptBin[uiBinIdx]);
	if(EmptyBin(uiBinIdx))
		ClearBinBitMap(uiBinIdx);
	
	return(p);
}
/*�ͷ�fastbins��ÿ��bin�µĿ���chunk*/
/*����ÿ��chunk��̽�Ƿ���Ժ��ڴ����ǰ��chunk�ϲ����ϲ�������ԣ���������chunkΪʹ��״̬������bin�����������������chunk�����unsortedbin��*/
int CBinMalloc::FreeFast()
{
	if(!(m_pstHead->m_uiFlags & MALLOC_FLAG_FAST)) // no fast chunk
		return(0);
	
	for(int i=0; i<NFASTBINS; i++){
		if(m_ptFastBin[i].m_hNextChunk != INVALID_HANDLE){
			CMallocChunk* pstChunk;
//			CMallocChunk* pstPreChunk;
			CMallocChunk* pstNextChunk;
			ALLOC_SIZE_T tSize;
			ALLOC_SIZE_T tPreSize;
//			ALLOC_SIZE_T tNextSize;
			unsigned int uiBinIdx;
			
			do{ // free fast-chunk & put it into unsorted chunk list
				pstChunk = (CMallocChunk*)Handle2Ptr(m_ptFastBin[i].m_hNextChunk);
				UnlinkBin(m_ptFastBin[i], m_ptFastBin[i].m_hNextChunk);

				tSize = CHUNK_SIZE(pstChunk);
				if(!prev_inuse(pstChunk) && CAN_COMBILE(tSize, pstChunk->m_tPreSize)){
					tPreSize = pstChunk->m_tPreSize;
					tSize += tPreSize;
					pstChunk = (CMallocChunk*)(((char*)pstChunk) - tPreSize);
					
					uiBinIdx = bin_index(tPreSize);
					UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstChunk));
					if(EmptyBin(uiBinIdx))
						ClearBinBitMap(uiBinIdx);
					set_inuse_bit_at_offset(pstChunk, tSize);
				}


				if(!AT_TOP(pstChunk, tSize)){
					pstNextChunk = (CMallocChunk*)(((char*)pstChunk) + tSize);
					ALLOC_SIZE_T tNextSize = CHUNK_SIZE(pstNextChunk);
					uiBinIdx = bin_index(tNextSize);
					if(!inuse_bit_at_offset(pstNextChunk, tNextSize) && CAN_COMBILE(tSize, tNextSize)){
						tSize += tNextSize;
						UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstNextChunk));
						if(EmptyBin(uiBinIdx))
							ClearBinBitMap(uiBinIdx);
						set_inuse_bit_at_offset(pstChunk, tSize);
					}
					else{
//						clear_inuse_bit_at_offset(pstNextChunk, 0);
					}
				}
	
				if(m_pstHead->m_tLastFreeChunkSize < REAL_SIZE(tSize))
					m_pstHead->m_tLastFreeChunkSize = REAL_SIZE(tSize);
				pstChunk->m_tSize = REAL_SIZE(tSize) | (pstChunk->m_tSize & SIZE_BITS);
				if(AT_TOP(pstChunk, tSize)){
					// combine into bottom
					m_pstHead->m_hTop -= tSize;
					statMemoryTop = m_pstHead->m_hTop;
//					clear_inuse_bit_at_offset(pstChunk, 0);
				}
				else{
					LinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstChunk));
				}
				pstNextChunk = (CMallocChunk*)(((char*)pstChunk) + REAL_SIZE(tSize));
				pstNextChunk->m_tPreSize = REAL_SIZE(tSize);
				
			}while(m_ptFastBin[i].m_hNextChunk != INVALID_HANDLE);
		}
	}
	
	m_pstHead->m_uiFlags &= ~MALLOC_FLAG_FAST;
	
	return(0);
}
/*��top���������һ��chunk����tsize*/
void* CBinMalloc::TopAlloc(ALLOC_SIZE_T tSize)
{
	if(m_pstHead->m_hTop + tSize + MINSIZE >= m_pstHead->m_tSize){
		snprintf(m_szErr, sizeof(m_szErr), "out of memory");
		return(NULL);
	}
	
	void* p;
	CMallocChunk* pstChunk;
	pstChunk = (CMallocChunk*)Handle2Ptr(m_pstHead->m_hTop);
	pstChunk->m_tSize = (pstChunk->m_tSize & SIZE_BITS) | REAL_SIZE(tSize);
	p = (void*)pstChunk;
	
	pstChunk = (CMallocChunk*)(((char*)pstChunk) + tSize);
	pstChunk->m_tPreSize = REAL_SIZE(tSize);
	pstChunk->m_tSize = PREV_INUSE;
	
	m_pstHead->m_hTop += tSize;
	statMemoryTop = m_pstHead->m_hTop;
	
	return chunk2mem(p);
}
/*�������bin�Ͻ�handleָ����chunk����*/
int CBinMalloc::UnlinkBin(CBin& stBin, INTER_HANDLE_T hHandle)
{
	CMallocChunk* pstChunk;
	CMallocChunk* pstTmp;
	
	if(hHandle == INVALID_HANDLE)
		return(-1);
	
	if(stBin.m_hNextChunk == INVALID_HANDLE || stBin.m_hPreChunk == INVALID_HANDLE){
		snprintf(m_szErr, sizeof(m_szErr), "unlink-bin: bad bin!");
		return(-2);
	}
	
	pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
	if(pstChunk->m_hPreChunk == INVALID_HANDLE){
		//remove head
		stBin.m_hNextChunk = pstChunk->m_hNextChunk;
	}
	else{
		pstTmp = (CMallocChunk*)Handle2Ptr(pstChunk->m_hPreChunk);
		pstTmp->m_hNextChunk = pstChunk->m_hNextChunk;
	}
	if(pstChunk->m_hNextChunk == INVALID_HANDLE){
		stBin.m_hPreChunk = pstChunk->m_hPreChunk;
	}
	else{
		pstTmp = (CMallocChunk*)Handle2Ptr(pstChunk->m_hNextChunk);
		pstTmp->m_hPreChunk = pstChunk->m_hPreChunk;
	}
	
	return(0);
}
/*��handleָ����chunk���뵽bin��*/
int CBinMalloc::LinkBin(CBin& stBin, INTER_HANDLE_T hHandle)
{
	CMallocChunk* pstChunk;
	CMallocChunk* pstTmp;
	
	if(hHandle == INVALID_HANDLE)
		return(-1);
	
	pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
	pstChunk->m_hNextChunk = stBin.m_hNextChunk;
	pstChunk->m_hPreChunk = INVALID_HANDLE;
	if(stBin.m_hNextChunk != INVALID_HANDLE){
		pstTmp = (CMallocChunk*)Handle2Ptr(stBin.m_hNextChunk);
		pstTmp->m_hPreChunk = hHandle;
		if(stBin.m_hPreChunk == INVALID_HANDLE){
			snprintf(m_szErr, sizeof(m_szErr), "link-bin: bad bin");
			return(-2);
		}
	}
	else{
		if(stBin.m_hPreChunk != INVALID_HANDLE){
			snprintf(m_szErr, sizeof(m_szErr), "link-bin: bad bin");
			return(-3);
		}
		stBin.m_hPreChunk = hHandle;
	}
	stBin.m_hNextChunk = hHandle;
	
	return(0);
}
/*��bin�в���һ�����ʵ�λ�ã���hanldeָ����chunk�����ȥ*/
/*Ѱ��λ�õķ�������bin��β����ʼ���ҵ���һ��λ�ã����Ĵ�С����ǰ��chunk�Ĵ�С֮��*/
int CBinMalloc::LinkSortedBin(CBin& stBin, INTER_HANDLE_T hHandle, ALLOC_SIZE_T tSize)
{
	CMallocChunk* pstChunk;
	CMallocChunk* pstNextChunk;
	
	if(hHandle == INVALID_HANDLE)
		return(-1);
	
	pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
	pstChunk->m_hNextChunk = INVALID_HANDLE;
	pstChunk->m_hPreChunk = INVALID_HANDLE;	
	
	if(stBin.m_hNextChunk == INVALID_HANDLE){ // empty bin
		pstChunk->m_hPreChunk = INVALID_HANDLE;
		pstChunk->m_hNextChunk = INVALID_HANDLE;
		stBin.m_hNextChunk = hHandle;
		stBin.m_hPreChunk = hHandle;
	}
	else{
		INTER_HANDLE_T hPre;
		hPre = stBin.m_hPreChunk;
		tSize = REAL_SIZE(tSize) | PREV_INUSE;
		CMallocChunk* pstPreChunk = 0;
		while(hPre != INVALID_HANDLE){
			pstPreChunk = (CMallocChunk*)Handle2Ptr(hPre);
			if(tSize <= pstPreChunk->m_tSize)
				break;
			hPre = pstPreChunk->m_hPreChunk;
		}
		if(hPre == INVALID_HANDLE){
			if(stBin.m_hPreChunk == INVALID_HANDLE){
				// empty list
				snprintf(m_szErr, sizeof(m_szErr), "memory corruction");
				return(-1);
			}
			
			// place chunk at list head
			LinkBin(stBin, hHandle);
		}
		else{
			pstChunk->m_hPreChunk = hPre;
			pstChunk->m_hNextChunk = pstPreChunk->m_hNextChunk;
			pstPreChunk->m_hNextChunk = hHandle;
			if(pstChunk->m_hNextChunk != INVALID_HANDLE){
				pstNextChunk = (CMallocChunk*)Handle2Ptr(pstChunk->m_hNextChunk);
				pstNextChunk->m_hPreChunk = Ptr2Handle(pstChunk);
			}
			else{
				// list tail
				stBin.m_hPreChunk = hHandle;
			}
		}					
	}
				
	return(0);
}
/*����chunk����tsize�������߼�*/
/*���岽���newmanwang��TTC�ڴ�����ʵ�ֵķ���*/
ALLOC_HANDLE_T CBinMalloc::InterMalloc(ALLOC_SIZE_T tSize)
{
	void* p;
	
	checked_request2size(tSize, tSize);

	/* no more use fast bin
	if(tSize < FAST_MAX_SIZE){
		p = FastMalloc(tSize);
		if(p != NULL)
			return Ptr2Handle(chunk2mem(p));
	}
	*/
	
	if(in_smallbin_range(tSize)){
		p = SmallBinMalloc(tSize);
		if(p != NULL)
			return Ptr2Handle(chunk2mem(p));
	}
	/* no more use fast bin
	else{
		FreeFast();
	}
	*/

	for(;;){
		CMallocChunk* pstChunk = NULL;
		CMallocChunk* pstNextChunk = NULL;
		//ALLOC_SIZE_T tChunkSize = 0;

		/* disable unsorted bin, newman */
		/*
		while(m_ptUnsortedBin[0].m_hNextChunk != INVALID_HANDLE){
			pstChunk = (CMallocChunk*)Handle2Ptr(m_ptUnsortedBin[0].m_hNextChunk);
			
			tChunkSize = CHUNK_SIZE(pstChunk);
			if(in_smallbin_range(tChunkSize) && pstChunk->m_hNextChunk==INVALID_HANDLE && tChunkSize > (tSize+MINSIZE)){
				//unlink
				UnlinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstChunk));
				
				//split it into two chunks
				ALLOC_SIZE_T tRemainSize;
				CMallocChunk* pstRemainChunk;
				tRemainSize = tChunkSize - tSize;
				pstRemainChunk = (CMallocChunk*)(((char*)pstChunk)+tSize);
				LinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstRemainChunk));

				pstChunk->m_tSize = REAL_SIZE(tSize) | (pstChunk->m_tSize & SIZE_BITS);;
				pstRemainChunk->m_tSize = REAL_SIZE(tRemainSize) | PREV_INUSE;
				pstRemainChunk->m_tPreSize = tSize;
				((CMallocChunk*)(((char*)pstRemainChunk)+tRemainSize))->m_tPreSize = REAL_SIZE(tRemainSize);
				
				p = (void*)pstChunk;
				return Ptr2Handle(chunk2mem(p));
			}
			
			// remove from unsorted list
			UnlinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstChunk));

			// Take now instead of binning if exact fit 
			if(tChunkSize == tSize){
				set_inuse_bit_at_offset(pstChunk, tChunkSize);
				p = (void*)pstChunk;
				return Ptr2Handle(chunk2mem(p));
			}
			else{
				clear_inuse_bit_at_offset(pstChunk, tChunkSize);
			}
			
			// place chunk into bin
			if(in_smallbin_range(tChunkSize)){
				LinkBin(m_ptBin[smallbin_index(tChunkSize)], Ptr2Handle(pstChunk));
				SetBinBitMap(smallbin_index(tChunkSize));
			}
			else{
				int iIdx = largebin_index(tChunkSize);
				LinkSortedBin(m_ptBin[iIdx], Ptr2Handle(pstChunk), tChunkSize);
				SetBinBitMap(iIdx);
			}
		}
		*/

		unsigned int uiBinIdx = bin_index(tSize);
		if (!in_smallbin_range(tSize)) {
			INTER_HANDLE_T v = m_ptBin[uiBinIdx].m_hNextChunk;
			unsigned int try_search_count = 0;

			/* ÿ��bin���ֻ����100�Σ����ʧ����������һ��bin */
			while(v != INVALID_HANDLE && ++try_search_count < 100 )
			{

				pstChunk = (CMallocChunk*)Handle2Ptr(v);
				if(CHUNK_SIZE(pstChunk) >= tSize)
					break;

				v = pstChunk->m_hNextChunk;
			}

			if(!(v != INVALID_HANDLE && try_search_count < 100 ))
				goto SEARCH_NEXT_BIN; 


			ALLOC_SIZE_T tRemainSize;
			tRemainSize = CHUNK_SIZE(pstChunk) - tSize;
			// unlink
			UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstChunk));
			if(EmptyBin(uiBinIdx))
				ClearBinBitMap(uiBinIdx);

			if(tRemainSize < GetMinChunkSize()){
				set_inuse_bit_at_offset(pstChunk, CHUNK_SIZE(pstChunk));
			}
			else{
				//split
				/*
				pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize);
				LinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstNextChunk));

				pstChunk->m_tSize =  tSize | (pstChunk->m_tSize & SIZE_BITS);
				pstNextChunk->m_tSize = tRemainSize;
				pstNextChunk->m_tPreSize = tSize;
				set_inuse_bit_at_offset(pstNextChunk, 0);
				set_inuse_bit_at_offset(pstNextChunk, tRemainSize);
				pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize+tRemainSize);
				pstNextChunk->m_tPreSize = tRemainSize;
				*/
			    pstChunk->m_tSize =  tSize | (pstChunk->m_tSize & SIZE_BITS);
			    pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize);
			    pstNextChunk->m_tSize = tRemainSize;
			    pstNextChunk->m_tPreSize = tSize;
			    set_inuse_bit_at_offset(pstNextChunk, 0);
			    ((CMallocChunk*)(((char*)pstChunk)+tSize+tRemainSize))->m_tPreSize = tRemainSize;
			    set_inuse_bit_at_offset(pstNextChunk, tRemainSize);
			    ALLOC_SIZE_T user_size;
			    InterFree(chunkhandle2memhandle(Ptr2Handle(pstNextChunk)), user_size);
			}

			p = (void*)pstChunk;
			return Ptr2Handle(chunk2mem(p));
		}

		/*
		   Search for a chunk by scanning bins, starting with next largest
		   bin. This search is strictly by best-fit; i.e., the smallest
		   (with ties going to approximately the least recently used) chunk
		   that fits is selected.
		   */
SEARCH_NEXT_BIN:
		uiBinIdx++;
		unsigned int uiBitMapIdx = uiBinIdx/32;
		if(m_pstHead->m_auiBinBitMap[uiBitMapIdx] == 0){
			uiBitMapIdx++;
			uiBinIdx = uiBitMapIdx*32;
			while(uiBitMapIdx<sizeof(m_pstHead->m_auiBinBitMap) && m_pstHead->m_auiBinBitMap[uiBitMapIdx] == 0){
				uiBitMapIdx++;
				uiBinIdx += 32;
			}
		}
		while(uiBinIdx < NBINS && m_ptBin[uiBinIdx].m_hNextChunk==INVALID_HANDLE)
			uiBinIdx++;

		if(uiBinIdx >= NBINS){
			goto MALLOC_BOTTOM;
		}

		INTER_HANDLE_T hPre;
		hPre = m_ptBin[uiBinIdx].m_hPreChunk;
		do{
			pstChunk = (CMallocChunk*)Handle2Ptr(hPre);
			hPre = pstChunk->m_hPreChunk;
		}while(CHUNK_SIZE(pstChunk) < tSize);
		ALLOC_SIZE_T tRemainSize;
		tRemainSize = CHUNK_SIZE(pstChunk) - tSize;
		// unlink
		UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstChunk));
		if(EmptyBin(uiBinIdx))
			ClearBinBitMap(uiBinIdx);

		if(tRemainSize < GetMinChunkSize()){
			set_inuse_bit_at_offset(pstChunk, CHUNK_SIZE(pstChunk));
		}
		else{
		    /*
		    //split
		    pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize);
		    LinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstNextChunk));

		    pstChunk->m_tSize = tSize | (pstChunk->m_tSize & SIZE_BITS);
		    pstNextChunk->m_tSize = tRemainSize;
		    pstNextChunk->m_tPreSize = tSize;
		    set_inuse_bit_at_offset(pstNextChunk, 0);
		    set_inuse_bit_at_offset(pstNextChunk, tRemainSize);
		    pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize+tRemainSize);
		    pstNextChunk->m_tPreSize = tRemainSize;
		     */

		    /* disable unsorted bins, newman */
		    pstChunk->m_tSize = tSize | (pstChunk->m_tSize & SIZE_BITS);
		    pstNextChunk = (CMallocChunk*)(((char*)pstChunk)+tSize);
		    pstNextChunk->m_tSize = tRemainSize;
		    pstNextChunk->m_tPreSize = tSize;
		    set_inuse_bit_at_offset(pstNextChunk, 0);
		    ((CMallocChunk*)(((char*)pstChunk)+tSize+tRemainSize))->m_tPreSize = tRemainSize;
		    set_inuse_bit_at_offset(pstNextChunk, tRemainSize);
		    ALLOC_SIZE_T user_size;
		    InterFree(chunkhandle2memhandle(Ptr2Handle(pstNextChunk)), user_size);
		}

		p = (void*)pstChunk;
		return Ptr2Handle(chunk2mem(p));
	}


MALLOC_BOTTOM:	
	return Ptr2Handle(TopAlloc(tSize));
}
/*��intermalloc�İ�װ���Է��ؽ�������˼򵥼��*/
ALLOC_HANDLE_T CBinMalloc::Malloc(ALLOC_SIZE_T tSize)
{
	CMallocChunk* pstChunk;

	m_pstHead->m_tLastFreeChunkSize = 0;
	ALLOC_HANDLE_T hHandle = InterMalloc(tSize);
	if(hHandle != INVALID_HANDLE){
		//		log_error("MALLOC: %lu", hHandle);
		pstChunk = (CMallocChunk*)mem2chunk(Handle2Ptr(hHandle));
		m_pstHead->m_tUserAllocSize += CHUNK_SIZE(pstChunk);
		m_pstHead->m_tUserAllocChunkCnt++;
		++statChunkTotal;
		statDataSize = m_pstHead->m_tUserAllocSize;
        AddAllocSizeToStat(tSize);
	}
	return(hHandle);
}
/*��intermalloc�İ�װ���Է��ؽ�������˼򵥼��,�������ص�chunk���û��������*/
ALLOC_HANDLE_T CBinMalloc::Calloc(ALLOC_SIZE_T tSize)
{
    ALLOC_HANDLE_T hHandle = Malloc(tSize);
    if(hHandle != INVALID_HANDLE)
    {
	char *p = Pointer<char>(hHandle);
	memset(p, 0x00, tSize);
    }

    return hHandle;
}

/*�������chunk��ʹ����ʱ�򷵻�0*/
int CBinMalloc::CheckInuseChunk(CMallocChunk* pstChunk)
{
	if(!inuse_bit_at_offset(pstChunk, CHUNK_SIZE(pstChunk))){
		snprintf(m_szErr, sizeof(m_szErr), "chunk not inuse!");
		return(-1);
	}
	
	CMallocChunk* pstTmp;
	if(!prev_inuse(pstChunk)){
		pstTmp = (CMallocChunk*)(((char*)pstChunk) - pstChunk->m_tPreSize);
		if(Ptr2Handle(pstTmp) < m_pstHead->m_hBottom || CHUNK_SIZE(pstTmp) != pstChunk->m_tPreSize){
			snprintf(m_szErr, sizeof(m_szErr), "invalid pre-chunk size!");
			return(-2);
		}
	}
	
	pstTmp = (CMallocChunk*)(((char*)pstChunk) + CHUNK_SIZE(pstChunk));
	if(!AT_TOP(pstTmp, 0)){
		if(CHUNK_SIZE(pstTmp) < MINSIZE){
			snprintf(m_szErr, sizeof(m_szErr), "invalid next chunk!");
			return(-3);
		}
	}
	
	return(0);
}
/*realloc�������߼�*/
/*�����߼���newmanwang��TTC�ڴ��������ʵ�ֵķ���*/
ALLOC_HANDLE_T CBinMalloc::InterReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize, ALLOC_SIZE_T& tOldMemSize)
{
	INTER_HANDLE_T hNewHandle;
	INTER_SIZE_T tNewSize;
	CMallocChunk* pstChunk;
	
	ALLOC_SIZE_T tUserReqSize = tSize;
	
	tOldMemSize = 0;
	if(hHandle == INVALID_HANDLE){
//		return InterMalloc(tSize - MALLOC_ALIGN_MASK);
		return InterMalloc(tSize);
	}
	
	if(tSize == 0){
		InterFree(hHandle, tOldMemSize);
		return(INVALID_HANDLE);
	}
	
	checked_request2size(tSize, tSize);
	
	if(hHandle >= m_pstHead->m_hTop || hHandle <= m_pstHead->m_hBottom){
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid handle");
		return(INVALID_HANDLE);
	}
	
	ALLOC_SIZE_T tOldSize;
	pstChunk = (CMallocChunk*)mem2chunk(Handle2Ptr(hHandle));
	tOldSize = CHUNK_SIZE(pstChunk);
	hHandle = Ptr2Handle((void*)pstChunk);
	if(hHandle + tOldSize >  m_pstHead->m_hTop){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid handle: %lu, size: %u", hHandle, tOldSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid handle: %llu, size: %u", hHandle, tOldSize);
#endif
		return(INVALID_HANDLE);
	}
	
	if(misaligned_chunk(hHandle)){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid handle: %lu, size: %u", hHandle, tOldSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid handle: %llu, size: %u", hHandle, tOldSize);
#endif
		return(INVALID_HANDLE);
	}
	
	if(tOldSize < MINSIZE){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid old-size: %lu, size: %u", hHandle, tOldSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid old-size: %llu, size: %u", hHandle, tOldSize);
#endif	
		return(INVALID_HANDLE);
	}
	
	if(CheckInuseChunk(pstChunk) != 0){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid chunk: %lu, size: %u", hHandle, tOldSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "realloc-invalid chunk: %llu, size: %u", hHandle, tOldSize);
#endif		
		return(INVALID_HANDLE);
	}
	tOldMemSize = tOldSize;
	
	int iPreInUse = prev_inuse(pstChunk);
	ALLOC_SIZE_T tPreSize = pstChunk->m_tPreSize;
	
	CMallocChunk* pstTmp;
	CMallocChunk* pstNextChunk;
	pstNextChunk = (CMallocChunk*)(((char*)pstChunk) + CHUNK_SIZE(pstChunk));
	
	if(tOldSize >= tSize){
		hNewHandle = hHandle;
		tNewSize = tOldSize;
	}
	else{
		/* Try to expand forward into top */
		if(AT_TOP(pstChunk, tOldSize) && m_pstHead->m_hTop + (tSize - tOldSize) + MINSIZE < m_pstHead->m_tSize){
			pstChunk->m_tSize = REAL_SIZE(tSize) | (pstChunk->m_tSize & SIZE_BITS);
			pstNextChunk = (CMallocChunk*)Handle2Ptr(m_pstHead->m_hTop + (tSize - tOldSize));
			pstNextChunk->m_tPreSize = REAL_SIZE(tSize);
			pstNextChunk->m_tSize = PREV_INUSE;
			
			m_pstHead->m_hTop += (tSize - tOldSize);
			statMemoryTop = m_pstHead->m_hTop;
			return Ptr2Handle(chunk2mem(pstChunk));
		}
		else if(!AT_TOP(pstChunk, tOldSize) && !inuse_bit_at_offset(pstNextChunk, CHUNK_SIZE(pstNextChunk)) && ((INTER_SIZE_T)tOldSize + CHUNK_SIZE(pstNextChunk)) >= tSize){
			hNewHandle = hHandle;
			tNewSize = (INTER_SIZE_T)tOldSize + CHUNK_SIZE(pstNextChunk);
			UnlinkBin(m_ptBin[bin_index(CHUNK_SIZE(pstNextChunk))], Ptr2Handle(pstNextChunk));
		}
		/* ada: defrag */
		else if(!prev_inuse(pstChunk) && (tOldSize + pstChunk->m_tPreSize) >= tSize){
			pstTmp = (CMallocChunk*)(((char*)pstChunk) - pstChunk->m_tPreSize);
			iPreInUse = prev_inuse(pstTmp);
			tPreSize = pstTmp->m_tPreSize;
			// copy & move
			hNewHandle = hHandle - pstChunk->m_tPreSize;
			tNewSize = (INTER_SIZE_T)tOldSize + pstChunk->m_tPreSize;
			UnlinkBin(m_ptBin[bin_index(pstChunk->m_tPreSize)], hNewHandle);
			// copy user data
			memmove(chunk2mem(Handle2Ptr(hNewHandle)), chunk2mem(Handle2Ptr(hHandle)), chunksize2memsize(tOldSize));
		}
		else{
			// alloc , copy & free
			hNewHandle = InterMalloc(tUserReqSize);
			if(hNewHandle == INVALID_HANDLE){
				snprintf(m_szErr, sizeof(m_szErr), "realloc-out of memory");
				return(INVALID_HANDLE);
			}
			pstTmp = (CMallocChunk*)mem2chunk(Handle2Ptr(hNewHandle));
			hNewHandle = Ptr2Handle(pstTmp);
			tNewSize = CHUNK_SIZE(pstTmp);
			// copy user data
			memcpy(chunk2mem(pstTmp), chunk2mem(Handle2Ptr(hHandle)), chunksize2memsize(tOldSize));
			ALLOC_SIZE_T tTmpSize;
			InterFree(chunkhandle2memhandle(hHandle), tTmpSize);
			return chunkhandle2memhandle(hNewHandle);
		}
	}
	
	assert(tNewSize >= tSize);
	CMallocChunk* pstNewChunk;
	pstNewChunk = (CMallocChunk*)Handle2Ptr(hNewHandle);
	INTER_SIZE_T tRemainderSize = tNewSize - tSize;
	if(tRemainderSize >= GetMinChunkSize()){
		// split
		CMallocChunk* pstRemainChunk;
		pstRemainChunk = (CMallocChunk*)(((char*)pstNewChunk)+tSize);
		ALLOC_SIZE_T tPreChunkSize = tSize;
		do{
			ALLOC_SIZE_T tThisChunkSize;
			if(tRemainderSize > MAX_ALLOC_SIZE){
				if(tRemainderSize - MAX_ALLOC_SIZE >= MINSIZE)
					tThisChunkSize = REAL_SIZE(MAX_ALLOC_SIZE);
				else
					tThisChunkSize = REAL_SIZE(tRemainderSize - MINSIZE);
			}
			else{
				tThisChunkSize = tRemainderSize;
			}
//			pstRemainChunk->m_tPreSize = tPreChunkSize;
			pstRemainChunk->m_tSize = REAL_SIZE(tThisChunkSize) | PREV_INUSE;
			
			// next chunk
			pstNextChunk = (CMallocChunk*)(((char*)pstRemainChunk) + REAL_SIZE(tThisChunkSize));
			pstNextChunk->m_tPreSize = REAL_SIZE(tThisChunkSize);
			pstNextChunk->m_tSize |= PREV_INUSE;	
			/* Mark remainder as inuse so free() won't complain */
			set_inuse_bit_at_offset(pstRemainChunk, tThisChunkSize);
			ALLOC_SIZE_T tTmpSize;
			InterFree(Ptr2Handle(chunk2mem(pstRemainChunk)), tTmpSize);
			
			tPreChunkSize = tThisChunkSize;
			tRemainderSize -= tThisChunkSize;
			pstRemainChunk = (CMallocChunk*)(((char*)pstRemainChunk)+REAL_SIZE(tThisChunkSize));
		}while(tRemainderSize > 0);
		
		tNewSize = tSize;
	}
	else{
		// next chunk
		pstNextChunk = (CMallocChunk*)(((char*)pstNewChunk) + REAL_SIZE(tNewSize));
//		pstNextChunk->m_tPreSize = REAL_SIZE(tNewSize);
		pstNextChunk->m_tSize |= PREV_INUSE;	
	}
	pstNewChunk->m_tSize = REAL_SIZE(tNewSize);
	if(iPreInUse)
		pstNewChunk->m_tSize |= PREV_INUSE;
	pstNewChunk->m_tPreSize = tPreSize;
		
	return Ptr2Handle(chunk2mem(pstNewChunk));
}
/*��intserrealloc�İ�װ���Է��ؽ�������˼򵥵ļ��*/
ALLOC_HANDLE_T CBinMalloc::ReAlloc(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T tSize)
{
	ALLOC_HANDLE_T hNewHandle;
	ALLOC_SIZE_T tOldSize;
	CMallocChunk* pstChunk;
	
	m_pstHead->m_tLastFreeChunkSize = 0;
	hNewHandle = InterReAlloc(hHandle, tSize, tOldSize);
	if(hNewHandle != INVALID_HANDLE){
		pstChunk = (CMallocChunk*)mem2chunk(Handle2Ptr(hNewHandle));
		m_pstHead->m_tUserAllocSize += CHUNK_SIZE(pstChunk);
		m_pstHead->m_tUserAllocSize -= tOldSize;
		if(hHandle == INVALID_HANDLE){
			m_pstHead->m_tUserAllocChunkCnt++;
			++statChunkTotal;
		}
        AddAllocSizeToStat(tSize);
		statDataSize = m_pstHead->m_tUserAllocSize;
	}
	else if(tSize == 0){
		m_pstHead->m_tUserAllocSize -= tOldSize;
		m_pstHead->m_tUserAllocChunkCnt--;
		--statChunkTotal;
		statDataSize = m_pstHead->m_tUserAllocSize;
	}
	
	return(hNewHandle);
}
/*free�ӿڵ������߼�*/
/*�����߼���newmanwang��TTC�ڴ��������ʵ�ֵķ���*/
int CBinMalloc::InterFree(ALLOC_HANDLE_T hHandle, ALLOC_SIZE_T& tMemSize)
{
	tMemSize = 0;
	if(hHandle == INVALID_HANDLE)
		return(0);
	
	if(hHandle >= m_pstHead->m_tSize){
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle");
		return(-1);
	}
	
//	log_error("FREE: %lu", hHandle);
	
	CMallocChunk* pstChunk;
	ALLOC_SIZE_T tSize;
	pstChunk = (CMallocChunk*)mem2chunk(Handle2Ptr(hHandle));
	tSize = CHUNK_SIZE(pstChunk);
	tMemSize = tSize;
	hHandle = Ptr2Handle((void*)pstChunk);
	if(hHandle + tSize >=  m_pstHead->m_tSize){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle: %lu, size: %u", hHandle, tSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle: %llu, size: %u", hHandle, tSize);
#endif	
		return(-2);
	}
	
	if(!inuse_bit_at_offset(pstChunk, tSize)){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "free-memory[handle %lu, size: %u, top: %lu] not in use", hHandle, tSize, m_pstHead->m_hTop);
#else
		snprintf(m_szErr, sizeof(m_szErr), "free-memory[handle %llu, size: %u, top: %llu] not in use", hHandle, tSize, m_pstHead->m_hTop);
#endif		
		return(-3);
	}

	if(misaligned_chunk(hHandle)){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle: %lu, size: %u", hHandle, tSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle: %llu, size: %u", hHandle, tSize);
#endif
		return(INVALID_HANDLE);
	}

	if(CheckInuseChunk(pstChunk) != 0){
#if __WORDSIZE == 64			
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid chunk: %lu, size: %u", hHandle, tSize);
#else
		snprintf(m_szErr, sizeof(m_szErr), "free-invalid chunk: %llu, size: %u", hHandle, tSize);
#endif		
		return(INVALID_HANDLE);
	}
	
	unsigned int uiBinIdx;
	CMallocChunk* pstNextChunk;
/*	
#ifdef USE_BIN_CACHE	
	if(tSize <= FAST_MAX_SIZE && (hHandle + tSize) != m_pstHead->m_hTop){
		uiBinIdx = fastbin_index(tSize);
		if(m_ptFastBin[uiBinIdx].m_hNextChunk == hHandle){
			snprintf(m_szErr, sizeof(m_szErr), "free-double free");
			return(-4);
		}
		LinkBin(m_ptFastBin[uiBinIdx], hHandle);
		
		if(m_pstHead->m_tLastFreeChunkSize < tSize)
			m_pstHead->m_tLastFreeChunkSize = tSize;
		m_pstHead->m_uiFlags |= MALLOC_FLAG_FAST;
		return(0);
	}
#endif
*/
	
	if(!prev_inuse(pstChunk) && CAN_COMBILE(tSize, pstChunk->m_tPreSize)){
		tSize += pstChunk->m_tPreSize;
		hHandle -= pstChunk->m_tPreSize;
		uiBinIdx = bin_index(pstChunk->m_tPreSize);
		pstChunk = (CMallocChunk*)(((char*)pstChunk) - pstChunk->m_tPreSize);
		// unlink
		UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstChunk));
		if(EmptyBin(uiBinIdx))
			ClearBinBitMap(uiBinIdx);
		set_size_at_offset(pstChunk, 0, tSize);
		set_presize_at_offset(pstChunk, tSize, tSize);
			
	}

	if((hHandle + tSize) != m_pstHead->m_hTop){
		pstNextChunk = (CMallocChunk*)Handle2Ptr(hHandle + tSize);
		if(CHUNK_SIZE(pstNextChunk) < MINSIZE){
			snprintf(m_szErr, sizeof(m_szErr), "free-invalid handle: "UINT64FMT_T", size: %u", hHandle, tSize);
			return(-4);
		}
		if(!inuse_bit_at_offset(pstNextChunk, REAL_SIZE(pstNextChunk->m_tSize)) && CAN_COMBILE(tSize, CHUNK_SIZE(pstNextChunk))){
			tSize += CHUNK_SIZE(pstNextChunk);
			uiBinIdx = bin_index(CHUNK_SIZE(pstNextChunk));
			// unlink
			UnlinkBin(m_ptBin[uiBinIdx], Ptr2Handle(pstNextChunk));
			if(EmptyBin(uiBinIdx))
				ClearBinBitMap(uiBinIdx);
			set_size_at_offset(pstChunk, 0, tSize);
			set_presize_at_offset(pstChunk, tSize, tSize);

		}
	}

	set_size_at_offset(pstChunk, 0, tSize);
	set_presize_at_offset(pstChunk, tSize, tSize);
	set_inuse_bit_at_offset(pstChunk, tSize);
	
	if(m_pstHead->m_tLastFreeChunkSize < tSize)
		m_pstHead->m_tLastFreeChunkSize = tSize;
	
	if((hHandle + tSize) == m_pstHead->m_hTop){
		m_pstHead->m_hTop -= tSize;
		statMemoryTop = m_pstHead->m_hTop;
		pstChunk->m_tSize = PREV_INUSE;
		if( m_pstHead->m_tSize > (m_pstHead->m_hTop + MINSIZE) && m_pstHead->m_tLastFreeChunkSize < m_pstHead->m_tSize - m_pstHead->m_hTop - MINSIZE)
			m_pstHead->m_tLastFreeChunkSize = m_pstHead->m_tSize - m_pstHead->m_hTop - MINSIZE;
		return(0);
	}

	/*
#ifdef USE_BIN_CACHE	
	// place it into unsorted link
	LinkBin(m_ptUnsortedBin[0], Ptr2Handle(pstChunk));
#else
*/
	clear_inuse_bit_at_offset(pstChunk, tSize);
	
	// place chunk into bin
	if(in_smallbin_range(tSize)){
		LinkBin(m_ptBin[smallbin_index(tSize)], Ptr2Handle(pstChunk));
		SetBinBitMap(smallbin_index(tSize));
	}
	else{
#if 0
		/* ��һ��bin�¹ҽӵĽڵ�ǳ���ʱ����ΪҪ��������������ûỨ�Ѻܶ�cpuʱ�� by ada */
		int iIdx = largebin_index(tSize);
		LinkSortedBin(m_ptBin[iIdx], Ptr2Handle(pstChunk), tSize);
#endif
		LinkBin(m_ptBin[largebin_index(tSize)], Ptr2Handle(pstChunk));
		SetBinBitMap(largebin_index(tSize));
	}
//#endif

	return(0);
}
/*��interfree�İ�װ���Է��ؽ�������˼򵥼��*/
int CBinMalloc::Free(ALLOC_HANDLE_T hHandle)
{
	int iRet;
	ALLOC_SIZE_T tSize;
	
	tSize = 0;
	iRet = InterFree(hHandle, tSize);
	if(iRet == 0){
		m_pstHead->m_tUserAllocSize -= tSize;
		m_pstHead->m_tUserAllocChunkCnt--;
		--statChunkTotal;
		statDataSize = m_pstHead->m_tUserAllocSize;
	}
	
	return(iRet);
}
/*�������free��handleָ��chunk�ܹ���cache������ٿ����ڴ�*/
/*ǰ��ϲ�chunk���ܵ����ͷű�ָ��handle�Ĵ�С����Ŀռ�*/
unsigned CBinMalloc::AskForDestroySize(ALLOC_HANDLE_T hHandle)
{
	ALLOC_SIZE_T logic_size  = 0;
	ALLOC_SIZE_T physic_size = 0;
	ALLOC_HANDLE_T physic_handle = 0;

	CMallocChunk * current_chunk = 0;
	CMallocChunk * next_chunk = 0;


	if(INVALID_HANDLE == hHandle || hHandle >= m_pstHead->m_tSize)
		goto ERROR;

	/* physic pointer */
	current_chunk = (CMallocChunk *)mem2chunk(Handle2Ptr(hHandle));
	physic_size   = CHUNK_SIZE(current_chunk);
	logic_size    = chunksize2memsize(physic_size);
	physic_handle = Ptr2Handle((void *)current_chunk);

	/* start error check. */
	/* overflow */
	if(physic_handle + physic_size > m_pstHead->m_tSize)
		goto ERROR;

	/* current chunk is not inuse */
	if(!inuse_bit_at_offset(current_chunk, physic_size))
		goto ERROR;

	/* not aligned */
	if(misaligned_chunk(physic_handle))
		goto ERROR;

	/* */
	if(0 != CheckInuseChunk(current_chunk))
		goto ERROR;

	/* try combile prev-chunk */
	if(!prev_inuse(current_chunk) && CAN_COMBILE(physic_size, current_chunk->m_tPreSize))
	{
		physic_size += current_chunk->m_tPreSize;

		/* forward handle */
		physic_handle -= current_chunk->m_tPreSize;
		current_chunk = (CMallocChunk *)((char *)current_chunk - current_chunk->m_tPreSize);

	}

	/* try combile next-chunk */
	if(physic_handle + physic_size != m_pstHead->m_hTop)
	{
		next_chunk = (CMallocChunk *)(Handle2Ptr(physic_handle + physic_size));
		if(CHUNK_SIZE(next_chunk) < MINSIZE)
			goto ERROR;

		/* can combile */
		if(!inuse_bit_at_offset(next_chunk, CHUNK_SIZE(next_chunk)) && 
				CAN_COMBILE(physic_size, CHUNK_SIZE(next_chunk)))
		{
			physic_size += CHUNK_SIZE(next_chunk);
		}
	}

	/* �ͷŵ�top�߽磬�ϲ���һ����ڴ� */
	if(physic_handle + physic_size == m_pstHead->m_hTop)
	{
		ALLOC_SIZE_T physic_free = m_pstHead->m_tSize - m_pstHead->m_hTop - MINSIZE + physic_size;
		physic_size = physic_size < physic_free ? physic_free : physic_size;
	}

	return chunksize2memsize(physic_size);

ERROR:
	snprintf(m_szErr, sizeof(m_szErr), "found invalid handle, can't destroy");
	return 0;
}

ALLOC_SIZE_T CBinMalloc::LastFreeSize()
{
	FreeFast();
	
	return chunksize2memsize(m_pstHead->m_tLastFreeChunkSize);
}


/**************************************************************************
 * for test
 * dump all bins and chunks
 *************************************************************************/

/*�����е�bin��飺small&large bins, fast bins, unsorted bins*/
/*У�鷽����ÿ��bin���һ��˫���ѭ������*/
int CBinMalloc::DumpBins()
{
	int i;
    int count;
    uint64_t size;
	
	INTER_HANDLE_T hHandle;
	CMallocChunk* pstChunk;
    printf("dump bins\n");
	for(i=0; i<NBINS; i++){
		hHandle = m_ptBin[i].m_hNextChunk;
        count = 0;
        size = 0;
		if(hHandle != INVALID_HANDLE){
			do{

				pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
				if(pstChunk->m_hNextChunk != INVALID_HANDLE)
					hHandle = pstChunk->m_hNextChunk;
                size += CHUNK_SIZE(pstChunk);
                ++count;
			}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
		}
		if(m_ptBin[i].m_hPreChunk != hHandle){
			printf("bad bin[%d]", i);
			return(-1);
		}
        if (count)
        {
#if __WORDSIZE == 64            
            printf("bins[%d] chunk num[%d] size[%lu]\n",i,count,size);
#else
            printf("bins[%d] chunk num[%d] size[%llu]\n",i,count,size);
#endif
        }
	}

    printf("dump fast bins\n");
	for(i=0; i<NFASTBINS; i++){
		hHandle = m_ptFastBin[i].m_hNextChunk;
        count = 0;
		if(hHandle != INVALID_HANDLE){
			do{
				pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
				if(pstChunk->m_hNextChunk != INVALID_HANDLE)
					hHandle = pstChunk->m_hNextChunk;
                ++count;
			}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
		}
		if(m_ptFastBin[i].m_hPreChunk != hHandle){
			printf("bad fast-bin[%d]\n", i);
			return(-2);
		}	
        if (count)
        {
            printf("fast bins[%d] chunk num[%d]\n",i,count);
        }
	}
    printf("dump unsorted bins\n");
	hHandle = m_ptUnsortedBin[0].m_hNextChunk;
    count = 0;
	if(hHandle != INVALID_HANDLE){
		do{
			pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
                printf("%d\n",CHUNK_SIZE(pstChunk));
			if(pstChunk->m_hNextChunk != INVALID_HANDLE)
				hHandle = pstChunk->m_hNextChunk;
		}while(pstChunk->m_hNextChunk != INVALID_HANDLE);
	}
	if(m_ptUnsortedBin[0].m_hPreChunk != hHandle){
#if __WORDSIZE == 64
		printf("bad unsorted-bin[%d] %lu!=%lu\n", 0, m_ptUnsortedBin[0].m_hPreChunk, hHandle);
#else
		printf("bad unsorted-bin[%d] %llu!=%llu\n", 0, m_ptUnsortedBin[0].m_hPreChunk, hHandle);
#endif
		return(-3);
	}	
    printf("unsorted bins:chunk num[%d]\n",count);

	return(0);
}

int CBinMalloc::DumpMem()
{	
	INTER_HANDLE_T hHandle;
	CMallocChunk* pstChunk;
	ALLOC_SIZE_T tSize;

	tSize = 0;
    printf("DumpMem\n");
    hHandle =  m_pstHead->m_hBottom;
	while(hHandle < m_pstHead->m_hTop){
		pstChunk = (CMallocChunk*)Handle2Ptr(hHandle);
        printf("%d\t\t%d\n",CHUNK_SIZE(pstChunk),prev_inuse(pstChunk) );
		hHandle += CHUNK_SIZE(pstChunk);
	}

	return(0);
}

