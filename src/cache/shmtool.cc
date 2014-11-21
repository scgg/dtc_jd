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
#include "ttc_global.h"
#include "bin_malloc.h"

const char progname[] = "shmtool";
const char version[] = TTC_VERSION;
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
/* 将shm id指定的共享内存内容写如到指定文件的内容
 */
static int save(int id, char *filename) {
	struct shmid_ds ds;
	char *ptr;
	int fd;

	if(shmctl(id, IPC_STAT, &ds) < 0) {
		fprintf(stderr, "shmctl: id(%u): %m\n", id);
		return -1;
	}
	ptr = (char *)shmat(id, NULL, SHM_RDONLY);
	if((long)ptr == -1) {
		fprintf(stderr, "shmat: id(%u): %m\n", id);
		return -1;
	}
	fd = open64 (filename, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0666);
	if(fd == -1) {
		fprintf(stderr, "open64(\"%s\"): %m\n", filename);
		return -1;
	}

	uint64_t write_bytes = 0;
    int64_t once_bytes  = 0;
    while (1)
    {
	    once_bytes = write (fd, ptr + write_bytes, ds.shm_segsz - write_bytes);

        if (once_bytes == -1)
        {
            fprintf (stderr, "write(%s): %m", filename);
            break;
        }

        write_bytes += once_bytes;
        if (write_bytes == ds.shm_segsz)
        {
            break;
        }
    }

	close(fd);

	return 0;
}

/* 打开或者创建由key指定的共享内存，将指定文件的内容load入共享内存
 */
static int load(key_t k, char *filename) {
	char *ptr;
	int id;
	int fd = open64(filename, O_RDONLY, 0666);
	int64_t size;

	if(fd == -1) {
		fprintf(stderr, "open64(\"%s\"): %m\n", filename);
		return -1;
	}

	size = lseek64(fd, 0L, SEEK_END);

    id = shmget(k, size, 0666|IPC_CREAT|IPC_EXCL);
    if(id == -1) {
        fprintf(stderr, "shmget: key(%u): %m\n", k);
        return -1;
    }

    ptr = (char *)shmat(id, NULL, 0);
    if((long)ptr == -1) {
        fprintf(stderr, "shmat: id(%u): %m\n", id);
        return -1;
    }

    lseek64(fd, 0L, SEEK_SET);
    int64_t read_bytes = 0;
    int64_t once_bytes = 0;
    while (1)
    {
        once_bytes = read(fd, ptr + read_bytes, size - read_bytes);

        if (once_bytes == -1)
        {
            fprintf (stderr, "read(%s): %m", filename);
            break;
        }

        read_bytes += once_bytes;
        if (read_bytes == size)
        {
            break;
        }
    }

    close(fd);
    return 0;
}

/* 删除由shm id指定的共享内存
 */
static int del(key_t k) {
	int id;

	id = shmget(k, 0, 0);
	if(id == -1) {
		fprintf(stderr, "shmget: key(%u): %m\n", k);
		return -1;
	}

	if(shmctl(id, IPC_RMID, NULL) < 0) {
		fprintf(stderr, "shmctl: id(%u): %m\n", id);
		return -1;
	}
	return 0;
}

static int status(int id) {
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

	return 0;
}

static void usage(void)
{
	fprintf(stderr, "Usage: shmtool {save|load|saveid|status} {id|key|ttc} filename [size]\n");
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

	if(argc==4 && !strcmp(argv[1], "save")) {
		k = get_shm_id(argv[2]);
		if(k==0) return -1;
		id = shmget(k, 0, 0);
		if(id == -1) {
			fprintf(stderr, "shmget: key(%u): %m\n", k);
			return -1;
		}
		return save(id, argv[3]);
	} else if(argc==4 && !strcmp(argv[1], "saveid")) {
		id = atoi(argv[2]);
		return save(id, argv[3]);
	} else if(argc==4 && !strcmp(argv[1], "load")) {
		k = get_shm_id(argv[2]);
		if(k==0) return -1;
		return load(k, argv[3]);
	} else if(argc==3 && !strcmp(argv[1], "delete")) {
		k = get_shm_id(argv[2]);
		if(k==0) return -1;
		return del(k);
	} else if(argc==3 && !strcmp(argv[1], "status")) {
		k = get_shm_id(argv[2]);
		if(k==0) return -1;
		id = shmget(k, 0, 0);
		if(id == -1) {
			fprintf(stderr, "shmget: key(%u): %m\n", k);
			return -1;
		}
		return status(id);
	}
	usage();
	return -1;
}
