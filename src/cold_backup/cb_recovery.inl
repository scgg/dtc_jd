/*
 * =====================================================================================
 *
 *       Filename:  cb_recovery.inl
 *
 *    Description:  recovery cold backup log to server
 *
 *        Version:  1.0
 *        Created:  05/26/2009 11:05:29 AM
 *       Revision:  $Id: cb_recovery.inl 4809 2010-01-07 07:02:05Z samuelliao $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <errno.h>
#include <stdio.h>
#include "cb_recovery.h"

using namespace CB;

template <class T>
cb_recovery<T>::cb_recovery()
{
	_continous_error_count = 0;
	_recovery_node_count = 0;
}

template <class T>
cb_recovery<T>::~cb_recovery()
{
}

template <class T>
int cb_recovery<T>::open(const char *address, const char *backdir) 
{
	if(!(address && address[0]) || !(backdir && backdir[0]) ) {
		PRINT("input argument is invalid");
		return -1;
	}

	/* clone table definition */
	TTC::Server clone;
	clone.StringKey();
	clone.SetAddress(address);
	clone.SetTimeout(3); 
	clone.SetTableName("*");

	int ret =0;                     
	if((ret=clone.Ping()) != 0 && ret != -TTC::EC_TABLE_MISMATCH) {
		PRINT("connect slave [%s] failed, error code %d", address, ret);
		return -1;
	}

	PRINT("connected slave [%s]", address);

	_recovery_machine.SetAddress(address);
	_recovery_machine.SetTimeout(3);
	_recovery_machine.CloneTabDef(clone); 
	_recovery_machine.SetAutoUpdateTab(false);

#if 0
	// disabled by samuelliao, cold recovery should active hblog
	if(active_recovery_machine()) {
		PRINT("active slave failed, slave can't support admin command");
		return -1;
	}
#endif

	PRINT("active slave successfully");

	if(_logr.open(backdir)) {
		PRINT("%s", _logr.error_message());
		return -1;
	}

	return 0; 
}

template <class T>
int cb_recovery<T>::recovery_to_slave(CBinary key, CBinary value) 
{
	TTC::SvrAdminRequest request(&_recovery_machine);
	request.SetAdminCode(TTC::ReplaceRawData);

	request.EQ("key", key.ptr, key.len);
	request.Set("value", value.ptr, value.len);

	TTC::Result result;
	request.Execute(result);

	if(result.ResultCode() != 0) {
		return -1;
	}

	return 0;
}

/* 通过验证内存创建时间来激活备机hotbackup特性 */
template <class T>
int cb_recovery<T>::active_recovery_machine(void) 
{
	TTC::SvrAdminRequest request(&_recovery_machine);
	request.SetAdminCode(TTC::VerifyHBT);

	request.SetMasterHBTimestamp(0);
	request.SetSlaveHBTimestamp(0);

	TTC::Result result;
	request.Execute(result);

	if (result.ResultCode() != 0)
		return -1;

	return 0;
}


template <class T>
int cb_recovery<T>::run()
{

	PRINT("cold recovery start at %s", get_current_time_str());

	while(1) {
		/* 最大连续错误次数超过限制 */
		if(_continous_error_count > 10)
		{
			PRINT("continous error count exceeds threshold(10), shutdown");
			return -1;
		}
	

		int ret = _logr.read();

		/* read one record*/
		if (ret > 0) {
			if(_logr.decode() || _logr.size() != 2) {
				PRINT("data maybe corrupted, skip this record");
				++_continous_error_count;
				continue;
			}

			if(recovery_to_slave(_logr[0], _logr[1])) {
				PRINT("recovery to slave encounted error, skip this record");
				++_continous_error_count;
				continue;
			}

			++_recovery_node_count;

		/* encounted EOF */
		} else if ( ret == 0) {
			PRINT("recovery "UINT64FMT" node", _recovery_node_count);
			PRINT("cold recovery completed at %s", get_current_time_str());
			return 0;

		/* ERROR */
		} else{
			PRINT("logr.read error: %s", _logr.error_message());
			++_continous_error_count;
		}
	}
	
	return 0;
}

template <class T>
int cb_recovery<T>::close()
{
	return 0;
}

/* asctime() maybe append '\n' automatic, but I don't needed */
template <class T>
char * cb_recovery<T>::get_current_time_str()
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
