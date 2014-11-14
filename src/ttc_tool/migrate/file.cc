/*
 * =====================================================================================
 *
 *       Filename:  comm.cc
 *
 *    Description:  读写日志及锁操作等
 *
 *        Version:  1.0
 *        Created:  11/20/2010 10:44:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "file.h"
#include "log.h"
using namespace std;

string CFile::ReadFile(const char * name) 
{
	string ret;
	CMarkupSTL statusxml;
	if (!statusxml.Load(_filename.c_str())||!statusxml.FindElem("request"))
	{
		ret = "load file:"+_filename+"error or request info not found";
		return ret;
	}
	return statusxml.GetAttrib(name);
};

bool CFile::WriteFile(const char * name, string status) 
{
	bool ret = false;
	CMarkupSTL statusxml;

	if (!statusxml.Load(_filename.c_str())||!statusxml.FindElem("request"))
	{
		log_error("load file:%s error or request info not found", _filename.c_str());
		return false;
	}

	if (statusxml.GetAttrib(name).empty())
	{
		log_debug("add atribute name:%s status:%s", name, status.c_str());
		ret = statusxml.AddAttrib(name, status.c_str());
	}
	else
	{
		log_debug("update atribute name:%s status:%s", name, status.c_str());
		ret = statusxml.SetAttrib(name, status.c_str());
	}

	if (!ret)
	{
		log_error("Add or Set Attrib error");
		return false;
	}

	ret = statusxml.Save(_filename.c_str());
	log_debug("save modify to file:%s %s",_filename.c_str(), ret?"true":"false");
	return ret;
};


bool CFile::SetJID(CJournalID jid)
{
	char buf[128];
	snprintf(buf,sizeof(buf),"%u&%u", jid.serial, jid.offset);
	return WriteFile("jid", buf);
}

bool CFile::GetJID(CJournalID& jid)
{
	string tmp = ReadFile("jid");
	if (sscanf(tmp.c_str(), "%u&%u", &(jid.serial), &(jid.offset)) != 2)
	{
		log_error("read jid error:%s",tmp.c_str());
		return false;
	}
	return true;
}

bool CFile::SetStatus(MIGRATE_STATUS status)
{
	switch (status)
	{
		case NOT_STARTED:
			return WriteFile("status", "NOT_STARTED");
		case FULL_MIGRATING:
			return WriteFile("status", "FULL_MIGRATING");
		case FULL_ERROR:
			return WriteFile("status", "FULL_ERROR");
		case FULL_DONE:
			return WriteFile("status", "FULL_DONE");
		case INC_MIGRATING:
			return WriteFile("status", "INC_MIGRATING");
		case INC_ERROR:
			return WriteFile("status", "INC_ERROR");
		case INC_DONE:
			return WriteFile("status", "INC_DONE");
		case UNKONW_STATUS:
		default:
			return WriteFile("status", "UNKONW_STATUS");
	}
}

MIGRATE_STATUS CFile::GetStatus(void)
{
	string tmp = ReadFile("status");
	if (tmp == string("NOT_STARTED"))
		return NOT_STARTED;
	else if (tmp == string("FULL_MIGRATING"))
		return FULL_MIGRATING;
	else if (tmp == string("FULL_ERROR"))
		return FULL_ERROR;
	else if (tmp == string("FULL_DONE"))
		return FULL_DONE;
	else if (tmp == string("INC_MIGRATING"))
		return INC_MIGRATING;
	else if (tmp == string("INC_ERROR"))
		return INC_ERROR;
	else if (tmp == string("INC_DONE"))
		return INC_DONE;
	else
		return UNKONW_STATUS;
}

bool CFile::GetXML(CMarkupSTL& xml)
{
	if (!xml.Load(_filename.c_str()))
	{
		log_error("load file:%s error:%m",_filename.c_str());
		return false;
	}
	return true;

}
