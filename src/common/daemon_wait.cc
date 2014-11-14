#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/resource.h>

#include "config.h"
#include "log.h"
#include "daemon.h"
#include "memcheck.h"
#include <StatTTC.h>
#include "thread_cpu_stat.h"

void DaemonWait(void)
{
	CStatItemU32 statfd = statmgr.GetItemU32(SERVER_OPENNING_FD);
	statfd = 0;

	unsigned fdthreshold, fdlimit = DaemonGetFdLimit();
	if(fdlimit<0)
		fdlimit = 1024;
	fdthreshold = (fdlimit*80)/100;

	sleep(10);
	thread_cpu_stat cpu_stat;
	cpu_stat.init();
	while (!stop)
	{
		sleep (10);
		if(stop) break;

		cpu_stat.do_stat();

		/* 扫描进程打开的fd句柄数，如果超过配置阈值，向二级网管告警 */
		statfd = ScanProcessOpenningFD();
		if((unsigned)statfd > fdthreshold)
		{
			log_alert("openning fd overload[current=%d threshold=%d max=%d]",
					(unsigned)statfd, fdthreshold, fdlimit);
		}
#if MEMCHECK
		log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());
#endif
	}
}

/* 扫描进程已打开的文件句柄数 */
unsigned int ScanProcessOpenningFD(void)
{
	unsigned int count = 0;
	char fd_dir[1024] = {0};
	DIR * dir;      
	struct dirent * ptr;

	snprintf(fd_dir, sizeof(fd_dir), "/proc/%d/fd", getpid());

	if((dir =opendir(fd_dir)) == NULL)
	{       
		log_warning("open dir :%s failed, msg:%s", fd_dir, strerror(errno));
		return count;
	}   

	while((ptr = readdir(dir)) != NULL)
	{   
		if(strcasecmp(ptr->d_name, "..") != 0 && strcasecmp(ptr->d_name, ".") != 0)
			count ++;
	}   

	closedir(dir);
	return count;
}
