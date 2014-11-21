/*
 * =====================================================================================
 *
 *       Filename:  cb_syncer.inl
 *
 *    Description:  cb syncer 
 *
 *        Version:  1.0
 *        Created:  05/15/2009 02:41:51 PM
 *       Revision:  $Id: cb_syncer.inl 4809 2010-01-07 07:02:05Z samuelliao $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <time.h>

#include "cb_syncer.h"
#include "cb_writer.h"

using namespace CB;

template <class T>
cb_syncer<T>::cb_syncer()
{
	_step 	= 0;
	_start	= 0;
	_timeout = 0;
	_sync_node_count_stat = 0;
	bzero(_master_addr, sizeof(_master_addr));
	bzero(_errmsg, sizeof(_errmsg));
}

template <class T>
cb_syncer<T> ::~cb_syncer()
{
}

template <class T>
int cb_syncer<T> ::open(const char *addr,  int step , unsigned timeout, const char *dir, size_t slice)
{
	if(!(addr && addr[0])){
		PRINT("master address is invalid");
		return -1;
	}
	
	strncpy(_master_addr, addr, sizeof(_master_addr));


	// [1 5000]
	_step = step <= 0 ? 1 : step >= 5000 ? 5000 : step;
	PRINT("sync step: %d", _step);

	// [1*60*60 24*60*60] s
	_timeout = timeout <= 1*60*60 ? 1*60*60 : timeout >= 24*60*60 ? 24*60*60 : timeout;
	PRINT("sync timeout: %u s", (unsigned)_timeout);

	// [1G  ...)  bytes
	slice = slice <= 1000000000UL ? 1000000000UL : slice;
	PRINT("log slice size: %u", (unsigned)slice);


	if(_logw.open(dir, slice)) {
		PRINT("init logw failed, %s", _logw.error_message());
		return -1;
	}

	if(detect_master_online()) {
		return -1;
	}

#if 0
	// disabled by samuelliao, coldbackup shouldn't active hblog
	if(active_master()) {
		return -1;
	}
#endif

	return 0;
}

template <class T>
int cb_syncer<T>::run(void)
{
	_start    = time(0);
	_timeout += _start;

	PRINT("cold backup start at %s", get_current_time_str()); 

	unsigned limit = 0;
	while(1) {

		TTC::SvrAdminRequest request(&_master);
		request.SetAdminCode(TTC::GetKeyList);
		TTC::Result result;

		request.Need("key");
		request.Need("value");
		request.Limit(limit, _step);

		int ret = request.Execute(result);
		
		if(ret == -TTC::EC_FULL_SYNC_COMPLETE) {
			PRINT("backup "UINT64FMT" node", _sync_node_count_stat);
			PRINT("cold backup completed at %s", get_current_time_str());
			return 0;
		} else if(ret != 0) {
			PRINT("sync range[%u, %u] failed: %s", 
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
			PRINT("sync task expired");
			return -1;
		}

		limit += _step;
	}

	return 0;
}

template <class T>
int cb_syncer<T> ::close()
{
	return 0;
}

template <class T>
int cb_syncer<T>::detect_master_online() 
{
	TTC::Server clone;
	clone.StringKey();
	clone.SetAddress(_master_addr);
	clone.SetTimeout(10);
	clone.SetTableName("*");

	int ret =0;
	if((ret=clone.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH) {
		PRINT("connect master [%s] failed, error code %d", _master_addr, ret);
		return -1;
	}

	_master.SetAddress(_master_addr);
	_master.SetTimeout(10);
	_master.CloneTabDef(clone);      /* 从master复制表结构 */
	_master.SetAutoUpdateTab(false);

	PRINT("connected master [%s]", _master_addr);
	return 0;
}


template <class T>
int cb_syncer<T> ::active_master()
{
	TTC::SvrAdminRequest request(&_master);
	request.SetAdminCode(TTC::RegisterHB);
	request.SetHotBackupID((uint64_t)0); /* full-sync */
	TTC::Result result;
	
	if(request.Execute(result) != -TTC::EC_FULL_SYNC_STAGE) {
		PRINT("active master failed: %s", result.ErrorMessage());
		return -1;
	}

	PRINT("active master successfully");
	return 0;
}

template <class T>
int cb_syncer<T> ::sync_task_expired()
{
	time_t now = time(0);
	return now >= _timeout;
}


template <class T>
int cb_syncer<T>::save_data(TTC::Result& result) 
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


		CBinary k, v;
		k.ptr = (char *)result.BinaryValue("key",   &(k.len));
		v.ptr = (char *)result.BinaryValue("value", &(v.len));


		/* write */
		_logw << k << v;

		/* commit */
		if(_logw.commit()) {
			PRINT("commit failed, %s, data maybe corrupted, but ignore", _logw.error_message());
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
template <class T>
char * cb_syncer<T>::get_current_time_str()
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
