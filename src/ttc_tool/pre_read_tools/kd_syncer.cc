
/*
 * =====================================================================================
 *
 *       Filename:  kd_syncer.cc
 *
 *    Description: key dump syncer 
 *
 *        Version:  1.0
 *        Created:  08/30/2009 09:16:11 AM
 *       Revision:  $Id$
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (), foryzhou@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <time.h>
#include <string.h>
#include "kd_syncer.h"
#include <iostream>

using namespace std;
using namespace KD;
kd_syncer::kd_syncer()
{
	_step 	= 0;
	_start	= 0;
	_timeout = 0;
	_sync_node_count_stat = 0;
	bzero(_master_addr, sizeof(_master_addr));
	bzero(_errmsg, sizeof(_errmsg));
}

kd_syncer ::~kd_syncer()
{
	if(fout)
	{
		fout.close();
	}
}

int kd_syncer::open(const char *addr,  int step , unsigned timeout, const char *filename)
{
	if(!(addr && addr[0])){
		PRINT("ttc master address is invalid");
		return -1;
	}
	
	strncpy(_master_addr, addr, sizeof(_master_addr));


	// [1 5000]
	_step = step <= 0 ? 1 : step >= 5000 ? 5000 : step;
	PRINT("keydump step: %d", _step);

	// [1*60*60 24*60*60] s
	_timeout = timeout <= 1*60*60 ? 1*60*60 : timeout >= 24*60*60 ? 24*60*60 : timeout;
	PRINT("keydump timeout: %u s", (unsigned)_timeout);

	
	fout.open(filename);
	if (!fout)
	{
		PRINT("init keydump file:%s failed",filename);
		return -1;
	}

	if(detect_master_online()) {
		return -1;
	}

	if(active_master()) {
		return -1;
	}


	return 0;
}

int kd_syncer::run(void)
{
	_start    = time(0);
	_timeout += _start;

	PRINT("keydump start at %s", get_current_time_str()); 

	unsigned limit = 0;
	while(1) {

		TTC::SvrAdminRequest request(&_master);
		request.SetAdminCode(TTC::GetKeyList);
		TTC::Result result;

		request.Need("key");
		request.Limit(limit, _step);

		int ret = request.Execute(result);

		if(ret == -TTC::EC_FULL_SYNC_COMPLETE) {
			PRINT("dump "UINT64FMT" keys", _sync_node_count_stat);
			PRINT("keydump completed at %s", get_current_time_str());
			return 0;
		} else if(ret != 0) {
			PRINT("keydump range[%u, %u] failed: %s", 
					limit,
					_step,
					result.ErrorMessage());
			/* skip error range */
			limit += _step;
			continue;
		}
		
		if(save_data(result)) {
			/* write log failed, abort */
			return -1;
		}

		if(sync_task_expired()) {
			PRINT("keydump task expired");
			return -1;
		}

		limit += _step;
	}

	return 0;
}

int kd_syncer::close()
{
	return 0;
}

int kd_syncer::detect_master_online() 
{
	TTC::Server clone;
	clone.StringKey();
	clone.SetAddress(_master_addr);
	clone.SetTimeout(10);
	clone.SetTableName("*");

	int ret =0;
	if((ret=clone.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH) {
		PRINT("connect ttc master [%s] failed, %s", 
				_master_addr,
				clone.ErrorMessage());
		return -1;
	}

	_master.SetAddress(_master_addr);
	_master.SetTimeout(10);
	_master.CloneTabDef(clone);      /* 从master复制表结构 */
	_master.SetAutoUpdateTab(false);

	PRINT("connected ttc master [%s]", _master_addr);
	return 0;
}


int kd_syncer::active_master()
{
	TTC::SvrAdminRequest request(&_master);
	request.SetAdminCode(TTC::RegisterHB);
	request.SetHotBackupID((uint64_t)0); /* full-sync */
	TTC::Result result;
	
	if(request.Execute(result) != -TTC::EC_FULL_SYNC_STAGE) {
		PRINT("active ttc master failed: %s", result.ErrorMessage());
		return -1;
	}

	PRINT("active ttc master successfully");
	return 0;
}

int kd_syncer::sync_task_expired()
{
	time_t now = time(0);
	return now >= _timeout;
}


int kd_syncer::save_data(TTC::Result& result) 
{
	static int error_count=0;

	/* ignore */
	if(result.NumRows() <= 0) {
		return 0;
	}

	for(int i=0; i < result.NumRows(); ++i) {
		if(result.FetchRow()) {
			PRINT("decode protocol failed, data maybe corrupted, %s",
					result.ErrorMessage());
			++error_count;
		}

		int *pkey =NULL;
		int len = 0;
		pkey =(int *) result.BinaryValue("key", &len);
		if(pkey&&len==4)//pkey not null,and len is 4(int)
		{
			try
			{
				/* write */
				fout << *pkey<<'\n'<<flush;
			}
			catch (ofstream::failure e)
			{
				/* commit */
				PRINT("commit failed, %s, data maybe corrupted, but ignore", e.what());
				++error_count;
			}
		}
		else if(len != 4)
		{
			cout<<"keydump only support int key now!\n"<<endl;
			++error_count;
		}
		else
		{
			cout<<"get key error\n"<<endl;
			++error_count;
		}
	
		/* max error count limit */
		if(error_count > 10) {
			PRINT("continous error count exceeds threshold(10), shutdown");
			return -1;
		}
		
		/* stat sync node count */
		++_sync_node_count_stat;
	}

	return 0;
}


/* asctime() maybe append '\n' automatic, but I don't needed */
char * kd_syncer::get_current_time_str()
{
	static char time_buffer[64];

	struct tm t;
	time_t now = time(0);
	localtime_r(&now, &t);
	snprintf(time_buffer, sizeof(time_buffer), "%04d-%02d-%02d %02d:%02d:%02d",
			t.tm_year+1900,
			t.tm_mon+1,
			t.tm_mday,
			t.tm_hour,
			t.tm_min,
			t.tm_sec
		);

	return time_buffer;
}
