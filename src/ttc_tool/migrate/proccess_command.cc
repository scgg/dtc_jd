/*
 * =====================================================================================
 *
 *       Filename:  proccess_command.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/13/2010 05:27:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <set>
#include "migrate_unit.h"
#include "proccess_command.h"
#include "log.h"
#include "unix_lock.h"
#include "migrate_main.h"

using namespace std;
//检查该任务对应的文件是否存在
bool CProcessCommand::IsTaskExist(void){return !access(_logfile_name.c_str(), F_OK);};
//根据任务的名字新建unix-socket锁
bool CProcessCommand::Lock(void) {return UnixSocketLock("migrate-agent-lock:%s",_logfile_name.c_str())>=0;};

void CProcessCommand::process()
{
	if (!recv_package())
		return;
	if(!decode())
		return;
	//if process command occus error, it should send error message
	process_command();

	close(_fd);
}

bool CProcessCommand::process_command(void)
{
	log_debug("name:%s command:%s",_logfile_name.c_str(),_command.c_str());
	bool ret = false;
	switch (parse_command())
	{
		case MIGRATE:
			ret = process_migrate();
			break;
		case REDO_MIGRATE:
			ret = process_redo_migrate();
			break;
		case RETRY_MIGRATE:
			ret = process_retry_migrate();
			break;
		case GET_STATUS:
			ret = process_get_status();
			break;
		default:
			log_error("unkown command:%s",_command.c_str());
			_error_msg = string("unknown command:")+_command;
			ret = false;
			break;
	}

	if (ret)
		return true;
	else
	{
		send_result(_fd,_error_msg);
		return false;
	}
	return true;
}

bool CProcessCommand::process_migrate(void)
{
	if (!Lock())
	{
		_error_msg = "task log file:"+_logfile_name+" is processing";
		return false;
	}

	if (IsTaskExist())
	{
		_error_msg = "task log file:"+_logfile_name+" is exist.please try retry or redo command";
		return false;
	}
	CMigrateUnit migrate_unit;
	int iret = migrate_unit.Init(&_xml);
	if (iret != 0)
	{
		char tmp_ret[1024];
		snprintf(tmp_ret,sizeof(tmp_ret),
				"init migrate unit error:%d",iret);
		_error_msg = tmp_ret;
		return false;
	}
	else
	{
		if (!_xml.Save(_logfile_name.c_str()))
		{
			log_error("create migrate log error");
			_error_msg = "create migrate log error";
			return false;
		}
		send_result(_fd,"ok");
		//全新开始
		if (migrate_unit.DoFullMigrate())
		{
			log_error("Do full migrate error");
			return false;
		}

		if (migrate_unit.DoIncMigrate())
		{
			log_error("Do inc migrate error");
			return false;
		}

	}
	return true;
}

bool CProcessCommand::process_redo_migrate(void)
{
	if (!Lock())
	{
		_error_msg = "task log file:"+_logfile_name+" is processing";
		return false;
	}

	if (!IsTaskExist())
	{
		_error_msg = "task log file:"+_logfile_name+" not exist..try migrate first";
		return false;
	}

	if (_log_file->GetStatus() == INC_DONE)
	{
		_error_msg = "task:"+_logfile_name+" is done..can't do any operate more";
		return false;
	}

	//重新使用目前已经存在的xml来运行process_migrate
	CMarkupSTL xml;
	if (!_log_file->GetXML(xml)) 
	{
		_error_msg = "load:"+_logfile_name+" error..";
		return false;
	}
	CMigrateUnit migrate_unit;
	int iret = migrate_unit.Init(&xml);
	if (iret != 0)
	{
		char tmp_ret[1024];
		snprintf(tmp_ret,sizeof(tmp_ret),
				"init migrate unit error:%d",iret);
		_error_msg = tmp_ret;
		return false;
	}
	else
	{
		send_result(_fd,"ok");
		//全新开始
		if (migrate_unit.DoFullMigrate())
		{
			log_error("Do full migrate error");
			return false;
		}

		if (migrate_unit.DoIncMigrate())
		{
			log_error("Do inc migrate error");
			return false;
		}

	}

	return true;
}

bool CProcessCommand::process_retry_migrate(void)
{
	if (!Lock())
	{
		_error_msg = "task log file:"+_logfile_name+" is processing";
		return false;
	}

	if (!IsTaskExist())
	{
		_error_msg = "task log file:"+_logfile_name+" not exist";
		return false;
	}

	if (_log_file->GetStatus() == INC_DONE)
	{
		_error_msg = "task:"+_logfile_name+" is done..can't do any operate more";
		return false;
	}

	CMarkupSTL xml;
	if (!_log_file->GetXML(xml)) 
	{
		_error_msg = "load:"+_logfile_name+" error..";
		return false;
	}
	CMigrateUnit migrate_unit;
	int iret = migrate_unit.Init(&xml);
	if (iret != 0)
	{
		char tmp_ret[1024];
		snprintf(tmp_ret,sizeof(tmp_ret),
				"init migrate unit error:%d",iret);
		_error_msg = tmp_ret;
		return false;
	}

	switch (_log_file->GetStatus())
	{
		case INC_DONE:
			_error_msg = "task has done:"+_logfile_name+"can't be retry";
			return false;
		case UNKONW_STATUS:
			_error_msg = "task status unknown:"+_logfile_name+"please check the file";
			return false;
		case NOT_STARTED:
		case FULL_MIGRATING:
		case FULL_ERROR: 
			send_result(_fd,"ok");
			//如果全量没有正确完成，重新全量
			if (migrate_unit.DoFullMigrate())
			{
				log_error("Do full migrate error");
				return false;
			}

			if (migrate_unit.DoIncMigrate())
			{
				log_error("Do inc migrate error");
				return false;
			}
			return true;
		case FULL_DONE:          
		case INC_MIGRATING:      
		case INC_ERROR:  
			//如果全量成功，直接增量
			if (migrate_unit.DoIncMigrate())
			{
				log_error("Do inc migrate error");
				return false;
			}
			return true;
	}

}

bool CProcessCommand::process_get_status(void)
{
	if (IsTaskExist())
	{
		send_result(_fd,_log_file->ReadFile("status").c_str());
		return true;
	}
	else
	{
		_error_msg = "no this task:"+_logfile_name;
		return false;
	}
}

bool CProcessCommand::recv_package()
{
	//命令包为xml格式，4个字节表示xml的长度,然后后面就是xml文件
	set_nonblocking(_fd);
	int recv_len = safe_tcp_read(_fd,_buf,4);
	if (recv_len != 4)
	{
		log_error("revieve package header error revcd:%d",recv_len);
		send_result(_fd,"revieve package error");
		return false;
	}
	_package_len = *(int32_t *)_buf;

	recv_len = safe_tcp_read(_fd,_buf,_package_len);
	if (recv_len != _package_len||recv_len >= MAX_PACKAGE_SIZE)
	{
		log_error("revieve package error revcd:%d expect:%d",recv_len,_package_len);
		send_result(_fd,"revieve package error");
		return false;
	}
	log_debug("len:%d buf:%.*s\n",_package_len,_package_len,_buf);
	return true;
}

bool CProcessCommand::decode()
{
	string request(_buf,_package_len);
	if(!_xml.SetDoc(request.c_str()))
	{
		log_error("decode package error:\n%s\n",request.c_str());
		send_result(_fd,"decode package error");
		return false;
	}

	_xml.ResetMainPos();

	if (!_xml.FindElem("request"))
	{
		log_error("no request info:\n%s\n",request.c_str());
		send_result(_fd,"no request info");
		return false;
	}

	_command = _xml.GetAttrib("command");
	_logfile_name = _xml.GetAttrib("name");

	if (_command.empty()||_logfile_name.empty())
	{
		log_error("no command info or name info");
		send_result(_fd,"no command or name info");
		return false;
	}
	_logfile_name = HISTORY_PATH + _logfile_name + ".xml";
	_log_file = new CFile(_logfile_name.c_str());
	return true;
}

CMD_TYPE CProcessCommand::parse_command(void)
{
	if (_command == string("migrate"))
		return MIGRATE;
	else if (_command == string("retry_migrate"))
		return RETRY_MIGRATE;
	else if (_command == string("redo_migrate"))
		return REDO_MIGRATE;
	else if (_command == string("get_status"))
		return GET_STATUS;
	else
		return UNKNOWN;
}
