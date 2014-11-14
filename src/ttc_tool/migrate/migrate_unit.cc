/*
 * =====================================================================================
 *
 *       Filename:  migrate_unit.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/11/2010 03:12:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "migrate_unit.h"
#include "log.h"
#include <stdint.h>
#include <errno.h>
#include "migrate_main.h"

using namespace std;

int CMigrateUnit::Init(CMarkupSTL* xml)
{
	xml->ResetMainPos();

	if (!xml->FindElem("request"))
	{
		log_error("no request info");
		return -1;
	}

	//获取请求的参数
	string _target_address = xml->GetAttrib("target");
	int timeout = atoi((xml->GetAttrib("timeout")).c_str());
	int step = atoi((xml->GetAttrib("step")).c_str());
	if (step>0)
		_step = step;
	int keytype = atoi((xml->GetAttrib("keytype")).c_str());
	string logfilename = HISTORY_PATH+xml->GetAttrib("name")+".xml";
	if (keytype>5||keytype<1)
	{
		log_error("bad keytype:%d",keytype);
		return -1;
	}
	//获取资源环及需要迁移的节点
	xml->ResetMainPos();
	if (!xml->FindChildElem("serverlist")||!xml->IntoElem())
	{
		log_error("can't find serverlist or enter it error");
		return -2;
	}
	pair<set<std::string>::iterator,bool> ret;
	while (xml->FindChildElem("server"))
	{
		if (!xml->IntoElem())
		{
			log_error("xml->IntoElem() error");
			return -3;
		}
		string name = xml->GetAttrib("name");
		string need_migrate = xml->GetAttrib("need_migrate");
		if (name.empty())
		{
			log_error("some server's name is empty");
			return -4;
		}

		ret = _serverlist.insert(name);
		if (ret.second == false)
		{
			log_error("server name duplicate. name:%s\n",name.c_str());
			return -5;
		}
		if (need_migrate == "yes")
		{
			ret = _filter.insert(name);
			if (ret.second == false)
			{
				log_error("insert to _filter error. name:%s\n",name.c_str());
				return -6;
			}
		}

		if (!xml->OutOfElem())
		{
			log_error("xml->OutElem() error");
			return -7;
		}
	}

	if (_serverlist.empty()||_filter.empty())
	{
		log_error("serverlist or need migrate list is empty");
		return -8;
	}

	_ttc = new CRequest(_target_address.c_str(), timeout, _step);
	log_debug("init CMigrateUnit success.\ntarget:%s\ntimeout:%d\nstep:%d\n",
			_target_address.c_str(),timeout,_step);
	_checker = new CKeyChecker(&_filter,&_serverlist,keytype);
	_logfile = new CFile(logfilename.c_str());
	return 0;
}

int CMigrateUnit::DoFullMigrate()
{
	int iret = DoFullMigrateInter();
	if (iret)
	{
		if (!_logfile->SetStatus(FULL_ERROR))
		{
			log_error("SetStatus to FULL_ERROR error:%m");
			return iret;
		}

	}
	else
	{
		if (!_logfile->SetStatus(FULL_DONE))
		{
			log_error("SetStatus to FULL_DONE error:%m");
			return iret;
		}
	}
	return iret;
}

int CMigrateUnit::DoFullMigrateInter()
{
	if (!_logfile->SetStatus(NOT_STARTED))
	{
		log_error("SetStatus to NOT_STARTED error");
		return -1;
	}
	log_debug("start do full migrate");
	CJournalID jid;
	if (_ttc->Regist(jid))
	{
		log_error("regist error");
		return -1;
	}
	else
	{
		if(!_logfile->SetJID(jid))
		{
			log_error("write jid to file error");
			return -1;
		}
	}

	int iret = SetTTCStatus(MS_MIGRATING);
	if (iret != 0)
	{
		log_error("set migrating error:%d",iret);
		return -1;
	}

	if (!_logfile->SetStatus(FULL_MIGRATING))
	{
		log_error("SetStatus to FULL_MIGRATING error");
		return -1;
	}

	int start=0;
	int step = 5000;
	while (1)
	{
		TTC::Result result;
		int ret = _ttc->GetKeyList(start,step,result);
		if (ret == 0)
		{
			if (BulkMigrate(result)<0)
			{
				log_error("full migrate error");
				return -1;
			}   
			start += step;
			continue;
		}
		else if (ret == 1)
		{
			log_debug("full migrate done");
			return 0;
		}
		else
		{
			log_error("get key list error");
			return ret;
		}
	}
}

int CMigrateUnit::DoIncMigrate()
{
	int iret = DoIncMigrateInter();
	if (iret)
	{
		if (!_logfile->SetStatus(INC_ERROR))
		{
			log_error("SetStatus to INC_ERROR error:%m");
			return iret;
		}

	}
	else
	{
		if (!_logfile->SetStatus(INC_DONE))
		{
			log_error("SetStatus to INC_ERROR error:%m");
			return iret;
		}
	}
	return iret;
}

int CMigrateUnit::DoIncMigrateInter()
{
	int step = 5000;
	log_debug("inc migrate start");
	int iret = 0;
	int migrate_keys_count = 0;
	int max_migrate_key_count = 0;
	CJournalID jid ;
	if (!_logfile->GetJID(jid))
	{
		log_error("get JID error,you may try redo migrate");
		return -1;
	}
	//增量
	//记下每次处理的最大值max_migrate_key_count
	//如果符合条件的key小于(max_migrate_key_count的1/10和10取中大的数)
	//我们跳出普通增量逻辑，进行后面的ReadOnly和收尾工作
	while (1)
	{
		TTC::Result result;
		int ret = _ttc->GetUpdateKeyList(step,jid,result);
		if (0 != ret) {//如果返回超时，说明没有增量key，出现其他错误则增量出错
			if (ret == -ETIMEDOUT)
			{
				log_debug ("no update data, err: %s, from: %s, limit: %d",
						result.ErrorMessage(), result.ErrorFrom(), step);
				migrate_keys_count = 0;
			}
			else
			{
				log_error ("fetch update-key-list failed, retry, err: %s, from: %s, limit: %d, iret:%d",
						result.ErrorMessage(), result.ErrorFrom(), step,ret);
				return -1;
			}
		}
		else//如果成功取出结果集中的增量key并且判断是否属于迁移范围并进行迁移
		{
			migrate_keys_count = BulkMigrate(result);
			if (migrate_keys_count < 0)
			{
				log_error("bulk migrate error:%d",migrate_keys_count);
				return -1;
			}
			else if (migrate_keys_count > max_migrate_key_count)
			{
				log_debug("max_migrate_key_count old:%d now:%d", max_migrate_key_count, migrate_keys_count);
				max_migrate_key_count = migrate_keys_count;
			}
			jid  = (uint64_t) result.HotBackupID();
			if (!_logfile->SetJID(jid))
			{
				log_error("set jid error,expect this migrate can done savely...and jid won't need anymore");
			}
		}

		if (migrate_keys_count <10 ||migrate_keys_count < 0.1*max_migrate_key_count)
			break;
	}

	//Set ReadOnly
	iret = SetTTCStatus(MS_MIGRATE_READONLY);
	if (iret != 0)
	{
		//if set readonly failed, we try to set it back to migrating...
		//because some node may set success,we must set it back...
		log_crit("Set ReadOnly Error:%d!!!!!!! try to set them back to migrating",iret,SetTTCStatus(MS_MIGRATING));
		return -1;
	}

	step = 10000;
	int timeout_count = 0;
	int timeout_retry_times = 3;
	while (1)
	{
		TTC::Result result;
		int ret = _ttc->GetUpdateKeyList(step,jid,result);
		if (0 != ret) {//如果返回超时，说明没有增量key，出现其他错误则增量出错
			if (ret == -ETIMEDOUT)
			{
				log_debug ("no update data, err: %s, from: %s, limit: %d",
						result.ErrorMessage(), result.ErrorFrom(), step);
				migrate_keys_count = 0;
				if (++timeout_count > timeout_retry_times)
				{
					break;
				}
			}
			else
			{
				log_error ("fetch update-key-list failed, retry, err: %s, from: %s, limit: %d, iret:%d",
						result.ErrorMessage(), result.ErrorFrom(), step,ret);
				iret = SetTTCStatus(MS_MIGRATING);
				if (iret != 0)
				{
					log_error("Send migrate completed cmd to server error:%d",iret);
					return -1;
				}
				return -1;
			}
		}
		else//如果成功取出结果集中的增量key并且判断是否属于迁移范围并进行迁移
		{
			migrate_keys_count = BulkMigrate(result);
			if (migrate_keys_count < 0)
			{
				log_error("bulk migrate error:%d",migrate_keys_count);
				iret = SetTTCStatus(MS_MIGRATING);
				if (iret != 0)
				{
					log_error("Send migrate completed cmd to server error:%d",iret);
					return -1;
				}
				return -1;
			}

			jid  = (uint64_t) result.HotBackupID();
			if (!_logfile->SetJID(jid))
			{
				log_error("set jid error,expect this migrate can done savely...and jid won't need anymore");
			}
			log_debug("get rows:%d expect rows:%d", result.NumRows(), step);
			if (result.NumRows() < step)
				break;
		}


	}
	iret = SetTTCStatus(MS_COMPLETED);
	if (iret != 0)
	{
		log_error("Send migrate completed cmd to server error:%d",iret);
		return -1;
	}
	log_debug("inc migrate done");
	return 0;
}

int CMigrateUnit::BulkMigrate(TTC::Result& result)
{
	int count = 0;
	int error_count = 0;
	log_debug("get %d keys",result.NumRows());
	if(result.NumRows() <= 0) {
		return 0;
	}
	const char * pkey = NULL;
	int pkeylen = 0;
	int iret = 0;

	for(int i=0; i < result.NumRows(); ++i) {
		if(result.FetchRow()) {
			log_error("decode protocol failed, data maybe corrupted, %s",
					result.ErrorMessage());
			return -1;
		}

		pkey = result.BinaryValue("key", &pkeylen);
		if (_checker->IsKeyInFilter(pkey,pkeylen))
		{
			iret = _ttc->MigrateOneKey(pkey,pkeylen);
			if (iret)
			{
				log_error("migrate onekey error:%d key:%d",iret,*(int *)pkey);
				return -1;
			}
			count++;
		}
	}
	log_debug("migrate %d keys",count);
	return count;
}

int CMigrateUnit::SetTTCStatus(int status){

	for(std::set<std::string>::const_iterator i = _filter.begin();
			i != _filter.end(); ++i) 
	{
		if (_ttc->SetStatus(i->c_str(),status)) 
		{
			log_error("set node[%s] status[%d] error", i->c_str(), status);
			return -1; 
		}
		log_debug("set node[%s] status[%d] succed", i->c_str(), status);
		return 0; 
	}
}
