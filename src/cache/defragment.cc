/*
 * =====================================================================================
 *
 *       Filename:  defragment.cc
 *
 *    Description:  内存整理器的实现
 *
 *        Version:  1.0
 *        Created:  07/19/2010 03:19:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <sys/types.h>
#include <sys/shm.h>
#include "defragment.h"
#include "raw_data.h"

#define PREV_INUSE 0x1
#define RESERVE_BITS (0x2|0x4)
#define SIZE_BITS (PREV_INUSE|RESERVE_BITS)
#define CHUNK_SIZE(p)         ((p)->m_tSize & ~(SIZE_BITS))
#define chunk2mem(h)   (void*)(((char*)h) + 2*sizeof(ALLOC_SIZE_T))
#define prev_inuse(p)       ((p)->m_tSize & PREV_INUSE)
extern int gMaxErrorCount;
#if __WORDSIZE == 64
# define UINT64FMT_T "%lu"
#else
# define UINT64FMT_T "%llu"
#endif


/*attach to memory and get ptr to CBinMalloc*/
/*
 * input:
 *      str:ttc key的字符串
 *      keysize:ttc key的长度，如果是定长则为实际长度，变长则为0
 *      step: 工具去操作ttc时的速度。0表示不限制,1-1000000表示每秒最多这么多次
 * output:
 *      0 成功
 *      其他表示失败
 *      具体参见错误日志*/

int CDefragment::Attach(const char * str,int keysize,int step)
{
    log_debug("shmkey:%s ttc keysize:%d",str,keysize);
    struct shmid_ds ds;
    char *ptr = NULL;
    int key = strtoul(str, NULL, 0);
    if(key==0)
    {
        log_error("bad shm key: [%s]\n", str);
        return -1;
    }
    int id = shmget(key, 0, 0);
    if(id == -1) {
        log_error("shmget: key(%u): %m\n", key);
        return -1;
    }

    if(shmctl(id, IPC_STAT, &ds) < 0) {
        log_error("shmctl: id(%u): %m\n", id);
        return -1;
    }
    ptr = (char *)shmat(id, NULL, SHM_RDONLY);
    if((long)ptr == -1) {
        log_error("shmat: id(%u): %m\n", id);
        return -1;
    }
    CMemHead *mh = (CMemHead *)ptr;
    _mem = CBinMalloc::Instance();
    if (_mem->Attach(ptr,mh->m_tSize))
    {
        log_error("bin malloc attach error\n");
        return -1;
    }
	_keysize = keysize;
	_bulk_per_ten_microscoends = step/100;
	if (_bulk_per_ten_microscoends < 1) _bulk_per_ten_microscoends = 1;
    return 0;
}

/*
 * 在一定程度可以通过handle定位内存块，然后找到ttc的key
 * 但是因为根据handle不能完全确定底层内存块存放的一定是数据，所以判断可能有误
 * 
 * input:
 *      handle:ttc内存结构中的chunk的handle
 *      len:key的实际长度将从这里返回
 * output:
 *      指向key的指针，如果出错返回NULL*/
char * CDefragment::GetKeyByHandle(INTER_HANDLE_T handle,int * len)
{
    char * key = NULL;
    CRawFormat * pstRaw;
    _pstChunk = (CMallocChunk*)_mem->Handle2Ptr(handle);
    pstRaw = (CRawFormat *)chunk2mem(_mem->Handle2Ptr(handle));
    if ((pstRaw->m_uchDataType != DATA_TYPE_RAW)    //不是DATA_TYPE_RAW，肯定不是数据块
            ||(pstRaw->m_uiDataSize > CHUNK_SIZE(_pstChunk))//数据大小比chunk大，肯定说明m_uiDataSize有问题，也就不是数据块
            ||(pstRaw->m_uiRowCnt > CHUNK_SIZE(_pstChunk))
            ||(pstRaw->m_uiRowCnt > pstRaw->m_uiDataSize))
        key = NULL;
    else
        key = pstRaw->m_achKey;

    if (key)
    {
        *len = _keysize == 0?(*(unsigned char *)key + 1):_keysize;
    }
    return key;

}
/*根据handle文件进行碎片整理。取出文件中的每一行，然后挨个处理
 * input:
 *       filename:文件名
 *       s:初始化好的TTC Server
 * output:
 *      0成功，其他失败，具体见log*/
int CDefragment::ProccessHandle(const char * filename,TTC::Server* s)
{
    if (s==NULL)
    {
        log_error("TTC Server is null");
        return -1;
    }
    _s =s;
    FILE * fp = fopen(filename,"r");
    if (fp == NULL)
    {
        log_error("open file:%s fault:%m",filename);
        return -1;
    }
    char buf[1024];
    while (fgets(buf,1024,fp))
    {
        Proccess(strtoull(buf,0,10));
    }
    fclose(fp);
    return 0;
}
/*处理一个handle，即根据输入的handle找到key，然后执行NodeHandleChange操作*/
int CDefragment::Proccess(INTER_HANDLE_T handle)
{
    int len;
    char * key = GetKeyByHandle(handle,&len);
    if (key == NULL)
    {
        log_debug("handle["UINT64FMT_T"] is not data chunk.skip it",handle);
        _skip_count++;
        return 0;
    }

	TTC::SvrAdminRequest request(_s);

	request.SetAdminCode(TTC::NodeHandleChange);
	request.EQ("key",key ,len);
	frequency_limit();
    TTC::Result result;
    int iRet = request.Execute(result);
    if (iRet == -ENOMEM)
    {
        log_crit("move chunk error,maybe one chunk lost");
        exit(0);
    }
    else if (iRet == -1032)
    {
	    log_debug("handle["UINT64FMT_T"] is not data chunk.skip it",handle);
	    _skip_count++;
	    return 0;
    }
    else if(iRet)
    {
	    log_error("handle["UINT64FMT_T"] request exe error: %d, error messgae: %s\n",handle,iRet, result.ErrorMessage());
	    _error_count++;
	    return -1;
    }
    log_debug("handle["UINT64FMT_T"] is execute ok",handle);
    _ok_count++;
    return 0;
}

/*
 * 整理内存
 * level:内存整理的级别，如果内存中空chunk之间有小于level个使用的chunk
 * 则将它们之间使用的chunk移开，使空chunk合并*/

int CDefragment::DefragMem(int level,TTC::Server* s)
{
	int error_count = 0;
	if (s==NULL)
	{
		log_error("TTC::server  is null");
		return -1;
	}
	_s = s;

    const CMemHead* pstHead = _mem->GetHeadInfo();
    if (pstHead == NULL)
    {
        log_error("pstHead is null");
        return -1;
    }

    INTER_HANDLE_T hHandle;
    INTER_HANDLE_T hHandle_next;

    CMallocChunk * pstNextChunk = NULL;
    CDefragMemAlgo Algo(level,this);
    hHandle =  pstHead->m_hBottom;
    //遍历内存chunk
    while(hHandle < pstHead->m_hTop){
        _pstChunk = (CMallocChunk*)_mem->Handle2Ptr(hHandle);
        hHandle_next = hHandle + CHUNK_SIZE(_pstChunk);
	pstNextChunk = (CMallocChunk*)_mem->Handle2Ptr(hHandle_next);

        //将当前chunk的指针及是否被使用的状态传给CDefragMemAlgo类，让它去处理
        if (Algo.Push(hHandle,(hHandle_next< pstHead->m_hTop)?prev_inuse(pstNextChunk):1)) 
        {
            log_error("Algo.Push error. handle["UINT64FMT_T"] handle_next["UINT64FMT_T"] prev_used[%d]",
                    hHandle,hHandle_next,(hHandle_next<pstHead->m_hTop)?prev_inuse(pstNextChunk):1);
            //just print log,not exit
        }
	if (CHUNK_SIZE(_pstChunk) <= 0) //sth may error 
	{
		if (error_count < gMaxErrorCount)
		{
			error_count++;
			log_error("PANIC:CHUNK_SIZE is zero,maybe sth wrong occur-_-.auto restart memdefragment..error/MAX ERROR:%d/%d",error_count,gMaxErrorCount);
			hHandle = pstHead->m_hBottom;
			continue;
		}
		else
		{
			log_error("PANIC:CHUNK_SIZE is zero,too many error occur-_-.memdefragment exit....");
			return -1;
		}
	}
        hHandle = hHandle_next;
    }
    log_info("DefragMem complete.skip["UINT64FMT_T"] ok["UINT64FMT_T"] error["UINT64FMT_T"]",
            _skip_count,_ok_count,_error_count);

    return 0;
}

int CDefragment::DefragMemNew(int level,TTC::Server* s,const char *filename,uint64_t memsize)
{
	FILE* fp = fopen(filename,"r");

	if (s==NULL)
	{
		log_error("TTC::server  is null");
		return -1;
	}
	_s = s;

	CDefragMemAlgo Algo(level,this);
	INTER_HANDLE_T handle;
	int used;
	int iret = 0;
	//用于显示百分比
	uint64_t base = 0;
	static uint64_t percent = 0;
	while (1)
	{
		iret = fread((char *)&handle,sizeof(handle),1,fp);
		if (iret <= 0)
		{
			log_info("read file:%s done:%m",filename);
			log_info("DefragMem complete.skip["UINT64FMT_T"] ok["UINT64FMT_T"] error["UINT64FMT_T"]",
					_skip_count,_ok_count,_error_count);
			return 0;
		}
		else if (iret != 1) 
		{
			log_error("read handle error:%d %m",iret);
			return -1;
		}

		iret = fread((char *)&used,sizeof(used),1,fp);
		if (iret<=0)
		{
			log_info("read file:%s done:%m",filename);
			return 0;
		}
		else if (iret != 1)
		{
			log_error("read used flag error:%d %m",iret);
			return -1;
		}

		log_debug("handle:"UINT64FMT_T" %d",handle,used);
		//将当前chunk的指针及是否被使用的状态传给CDefragMemAlgo类，让它去处理
		if (Algo.Push(handle,used)) 
		{
			log_error("Algo.Push error. handle["UINT64FMT_T"]",handle);
			//just print log,not exit
		}
		if (base == 0)
		{
			base = handle;
		}

		if ((100*(handle-base))/memsize == percent)
		{
			fprintf(stderr,"IN PROCCESS [ "UINT64FMT_T"/100 ]\n",percent++);
		}
	}

	fclose(fp);
	return 0;
}
/*遍历chunk，输出统计信息
 * verbose = true表示将详细信息输出到stderr
 * 如果为false则只输出简单的统计信息，默认为false*/

int CDefragment::DumpMem(bool verbose)
{
	int error_count = 0;
	const CMemHead* pstHead = _mem->GetHeadInfo();
	if (pstHead == NULL)
	{
		log_error("pstHead is null");
		return -1;
	}

	INTER_HANDLE_T hHandle;
	uint64_t usedChunk = 0;
	uint64_t unusedChunk = 0;
	int flag = 0;

	hHandle =  pstHead->m_hBottom;
	while(hHandle < pstHead->m_hTop){
		_pstChunk = (CMallocChunk*)_mem->Handle2Ptr(hHandle);
		prev_inuse(_pstChunk)?usedChunk++:unusedChunk++;
		if (verbose)
		{
			if (flag)
			{
				fprintf(stderr,"%d\n"UINT64FMT_T" %d ",prev_inuse(_pstChunk),hHandle,CHUNK_SIZE(_pstChunk));
			}
			else
			{
				fprintf(stderr,UINT64FMT_T" %d ",hHandle,CHUNK_SIZE(_pstChunk));
				flag++;
			}
		}
		if (CHUNK_SIZE(_pstChunk) <= 0) //sth may error 
		{
			if (error_count < gMaxErrorCount)
			{
				error_count++;
				log_error("PANIC:CHUNK_SIZE is zero,maybe sth wrong occur-_-.auto restart memdefragment..error/MAX ERROR:%d/%d",error_count,gMaxErrorCount);
				hHandle = pstHead->m_hBottom;
				continue;
			}
			else
			{
				log_error("PANIC:CHUNK_SIZE is zero,too many error occur-_-.memdefragment exit....");
				return -1;
			}
		}
		hHandle += CHUNK_SIZE(_pstChunk);
	}
	if (verbose)
		fprintf(stderr,"1\n");
	log_info("used chunknum:"UINT64FMT_T" unused chunknum:"UINT64FMT_T,usedChunk,unusedChunk);

	return(0);
}

int CDefragment::DumpMemNew(const char * filename, uint64_t& memsize)
{
	const CMemHead* pstHead = _mem->GetHeadInfo();
	if (pstHead == NULL)
	{
		log_error("pstHead is null");
		return -1;
	}
	FILE * fp = fopen(filename,"w");

	INTER_HANDLE_T hHandle;
	INTER_HANDLE_T hHandle_next;
	uint64_t usedChunk = 0;
	uint64_t unusedChunk = 0;
	CMallocChunk* pstNextChunk = NULL;
	int used = 0;
	int iret = 0;

	hHandle =  pstHead->m_hBottom;
	memsize = pstHead->m_hTop-pstHead->m_hBottom;
	while(hHandle < pstHead->m_hTop){
		_pstChunk = (CMallocChunk*)_mem->Handle2Ptr(hHandle);
		hHandle_next = hHandle + CHUNK_SIZE(_pstChunk);
		pstNextChunk = (CMallocChunk*)_mem->Handle2Ptr(hHandle_next);
		used = (hHandle_next< pstHead->m_hTop)?prev_inuse(pstNextChunk):1;
		prev_inuse(_pstChunk)?usedChunk++:unusedChunk++;
		iret = fwrite((void *)&hHandle,sizeof(hHandle),1,fp);
		if (iret != 1)
		{
			log_error("write to file:%s error:%m. handle["UINT64FMT_T"] handle_next["UINT64FMT_T"]",
					filename,hHandle,hHandle_next);
			return -1;
		}
		iret = fwrite((void *)&used,sizeof(used),1,fp);
		if (iret != 1)
		{
			log_error("write to file:%s error:%m. handle["UINT64FMT_T"] handle_next["UINT64FMT_T"]",
					filename,hHandle,hHandle_next);
			return -1;
		}

		if (CHUNK_SIZE(_pstChunk) <= 0) //sth may error 
		{
			log_error("PANIC:CHUNK["UINT64FMT_T"] is zero", hHandle);
			return -1;
		}
		hHandle += CHUNK_SIZE(_pstChunk);
	}
	log_info("used chunknum:"UINT64FMT_T" unused chunknum:"UINT64FMT_T,usedChunk,unusedChunk);
	fclose(fp);
	return(0);
}
CDefragMemAlgo::CDefragMemAlgo(int level,CDefragment * master)
{
	_count = 0;
	_queue = (INTER_HANDLE_T*)MALLOC(level*sizeof(INTER_HANDLE_T));
	_status = SEARCH;
	_level = level;
	_master = master;
	if (_queue == NULL)
	{
		log_error("MALLOC queue error");
		return;
	}
}

CDefragMemAlgo::~CDefragMemAlgo()
{
	FREE(_queue);
}

/*内存扫描算法*/
int CDefragMemAlgo::Push(INTER_HANDLE_T handle,int used)
{
	//chunk 使用状态必须为使用或者没使用，否则出错
	if (used !=0 &&  used !=1)
	{
		log_error("chunk flag must be 0|1,handle["UINT64FMT_T"] %d is invalid",handle,used);
		return -1;
	}

	/*初始化为SEARCH状态,因为BOTTOM的chunk肯定为已使用
	 * 当找到第一个未使用的chunk时进入MATCH状态，即查找连续的小于等于level个是否符合要求*/

	if (_status == SEARCH)
	{
		if (used == 1)
			return 0;
		_status = MATCH;
		return 0;
	}
	/*MATCH状态下:
	 * 如果遍历过的已使用chunk数小于level则继续遍历，并把handle存在queue里
	 * 如果大于level则表示本次匹配已经失败，回到SEARCH状态并清空queue
	 *
	 * 如果遇到空chunk
	 * 如果queue中有数据则将queue中的handle输出给Proccess去处理
	 * 如果queue中没有数据，表示有两个连续的空chunk，肯定是ttc内存分配算法哪里有问题
	 *
	 * 处理完后继续MATCH*/
	else if (_status == MATCH)
	{
		if (used == 1)
		{
			if (_count < _level)
			{
				_queue[_count++] = handle;
				return 0;
			}
			else
			{
				_count = 0;
				_status = SEARCH;
				return 0;
			}
		}
		else
		{
			if (_count == 0)
			{
				return -1;
			}
			else
			{
				for (int i=0;i<_count;i++)
					_master->Proccess(_queue[i]);
				_count = 0;
				return 0;
			}
		}
	}

	log_error("Unknow status:%d",_status);
	return -1;
}


void CDefragment::frequency_limit(void)
{
		static int count = 0;
		if (count++ == _bulk_per_ten_microscoends)
		{
				usleep(10000);//10ms
				count = 0;
				return;
		}
		else
		{
				return;
		}
}
