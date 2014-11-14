/*
 * =====================================================================================
 *
 *       Filename:  request.cc
 *
 *    Description:  本对象用于向TTC发送各种命令
 *
 *        Version:  1.0
 *        Created:  11/20/2010 11:22:00 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <errno.h>
#include <unistd.h>
#include "request.h"
#include "log.h"
void CRequest::frequency_limit(void)
{
	static int count = 0;
	if (count++ == _bulk_per_ten_microscoends)
	{
		count = 0;
		usleep(10000);//10ms
		return;
	}
	else
	{
		return;
	}
}

int CRequest::MigrateOneKey(const char *key,const int len)
{
	if (key == 0|| len == 0)
		return -EINVAL;
	int retry_count = 0;
	frequency_limit();
	while (1)
	{
		TTC::SvrAdminRequest request(&_server);
		request.SetAdminCode(TTC::Migrate);
		request.EQ("key",key,len);
		TTC::Result result;
		int ret = request.Execute(result);
		if (ret == 0 || ret == -TTC::EC_KEY_NOTEXIST)
			break;
		else{
			if (retry_count++ < _error_count)
			{
				log_debug("iret:%d retry_count:%d _error_count:%d",
						ret, retry_count, _error_count);
				usleep(100);
				continue;
			}
			else
			{
				log_error("migrate error, msg: %s, from:%s ret:%d\n",
						result.ErrorMessage(),
						result.ErrorFrom(),
						ret);
				return -1;
			}

		}
	}
	return 0;
}

int CRequest::SetStatus(const char * name, int state)
{
	int error_count = 0;
	int iret = 0;
	while(1)
	{
		TTC::SvrAdminRequest request(&_server);
		request.SetAdminCode(TTC::SetClusterNodeState);
		request.Set("key", name);
		request.Set("flag", state);

		TTC::Result r;
		iret = request.Execute(r);
		if (iret == 0)
		{
			return 0;
		}
		else if (error_count<_error_count)
		{
			error_count++;
			usleep(100);
			continue;
		}
		else
		{
			log_error("try %d times,errorinfo:%s %s\n", _error_count, r.ErrorFrom(), r.ErrorMessage());
			return iret;
		}
	}
}

int CRequest::Regist(CJournalID& jid)
{
	TTC::SvrAdminRequest request(&_server);
	request.SetAdminCode(TTC::RegisterHB);
	request.SetHotBackupID((uint64_t)0); /* full-sync */
	TTC::Result result;

	if(request.Execute(result) != -TTC::EC_FULL_SYNC_STAGE) {
		log_error("active ttc master failed: %s", result.ErrorMessage());
		return -1;
	}
	jid = result.HotBackupID();
	log_notice("serial:%u offset:%u",jid.serial,jid.offset);
	return 0;
};

int CRequest::GetKeyList(int start,int step,TTC::Result& result)
{
	log_debug("start:%d step:%d",start,step);
	TTC::SvrAdminRequest request(&_server);
	request.SetAdminCode(TTC::GetKeyList);
	request.Need("key");
	request.Limit(start, step);

	int ret = request.Execute(result);

	if(ret == -TTC::EC_FULL_SYNC_COMPLETE) {
		log_notice("full migrate done");
		return 1;
	} else if(ret != 0) {
		log_error("keydump range[%u, %u] failed: %s", 
				start,
				step,
				result.ErrorMessage());
		return ret;
	}
	else
		return 0;

}

int CRequest::GetUpdateKeyList(int step,CJournalID jid,TTC::Result& result)
{
	TTC::SvrAdminRequest request(&_server);
	request.SetAdminCode(TTC::GetUpdateKey);
	request.Need("key");
	request.SetHotBackupID((uint64_t) jid);

	request.Limit(0, step);

	int ret = request.Execute(result);
	if (-EBADRQC == ret) {
		log_error("server hot-backup not active yet");
		return ret;
	}
	else if (-TTC::EC_BAD_HOTBACKUP_JID == ret) {
		log_error("server report journalID is not match");
		return ret;
	}
	return ret;
}
