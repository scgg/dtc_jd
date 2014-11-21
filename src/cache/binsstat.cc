#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "config.h"
#include "version.h"
#include "daemon.h"
#include "mdetect.h"
#include "bin_malloc.h"

#define BINSONLY 0x01
#define MEMONLY 0x02

char cacheFile[256] = CACHE_CONF_NAME;
char tableFile[256] = TABLE_CONF_NAME;

#define _FILE_OFFSET_BITS 64

/* str -> shm key. 如果str没有指定key，则读取cache配置文件的key
 */
static int get_shm_id(const char *str)
{
	if(!str || !str[0])
		str = "ttc";

	if(isdigit(str[0]))
	{
	}
	else if(!strcasecmp(str, "ttc"))
	{
		CConfig gConfig;
		if(gConfig.ParseConfig(tableFile, "DB_DEFINE"))
			return 0;
		if(gConfig.ParseConfig(cacheFile, "cache"))
			return 0;
		str = gConfig.GetStrVal("cache", "CacheShmKey");
	}
	int key = strtoul(str, NULL, 0);
	if(key==0)
	{
		fprintf(stderr, "bad shm key: [%s]\n", str);
		return 0;
	}
	return key;
}

static int status(int id,int flag) {
	struct shmid_ds ds;
	char *ptr;

	if(shmctl(id, IPC_STAT, &ds) < 0) {
		fprintf(stderr, "shmctl: id(%u): %m\n", id);
		return -1;
	}
	ptr = (char *)shmat(id, NULL, SHM_RDONLY);
	if((long)ptr == -1) {
		fprintf(stderr, "shmat: id(%u): %m\n", id);
		return -1;
	}

	CMemHead *mh = (CMemHead *)ptr;
	double total = mh->m_tSize / 1048576.0;
	double alloc = mh->m_tUserAllocSize / 1048576.0;
	double top = mh->m_hTop / 1048576.0;
	double pct0 = 0;
	double pct1 = 0;

	if(mh->m_tSize > 0) {
		pct0 = 100.0 * alloc / total;
	}
	if(mh->m_tSize > 0) {
		pct1 = 100.0 * top / total;
	}

	printf("total %.2fM allocated %.2fM %.2f%% top %.2fM %.2f%%\n",
			total, alloc, pct0, top, pct1);
    CBinMalloc * b = CBinMalloc::Instance();
    if (b->Attach(ptr,mh->m_tSize))
    {
        fprintf(stderr,"bin malloc attach error\n");
        return -1;
    }
    if (flag&BINSONLY)
        b->DumpBins();
    if (flag&MEMONLY)
        b->DumpMem();


	return 0;
}

static void usage(void)
{
	fprintf(stderr, "Usage: binsstat {id|key|ttc} {bins|mem}\n");
	exit(-1);
}
/*  可以通过参数传入3中操作：save, load, delete
 *  1.save: 将共享内存的数据原封不动写入指定文件
 *  2.load: 将文件中的数据load入共享内存
 *  3.delete: 删除指定的共享内存
 */
int main(int argc, char **argv) {
	int id;
	key_t k;

	if(argc==2) {
		k = get_shm_id(argv[1]);
		if(k==0) return -1;
		id = shmget(k, 0, 0);
		if(id == -1) {
			fprintf(stderr, "shmget: key(%u): %m\n", k);
			return -1;
		}
        return status(id,BINSONLY|MEMONLY);
	}
    else if (argc==3)
    {
        k = get_shm_id(argv[1]);
        if(k==0) return -1;
        id = shmget(k, 0, 0);
        if(id == -1) {
            fprintf(stderr, "shmget: key(%u): %m\n", k);
            return -1;
        }
        if (!strcasecmp(argv[2],"bins"))
            return status(id,BINSONLY);
        else if (!strcasecmp(argv[2],"mem"))
            return status(id,MEMONLY);
        return status(id,BINSONLY|MEMONLY);
    }
	usage();
	return -1;
}
