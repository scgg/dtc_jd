/*
 * =====================================================================================
 *
 *       Filename:  migrate_unit.h
 *
 *    Description:  migrate工具的主框架
 *
 *        Version:  1.0
 *        Created:  11/11/2010 02:53:02 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _MIGRATE_UINT_H_
#define _MIGRATE_UINT_H_
#include "ttcapi.h"
#include "MarkupSTL.h"
#include "request.h"
#include "key_checker.h"
#include <string>
#include <set>
#include "file.h"
class CMigrateUnit
{
	public:
		CMigrateUnit()
		{
				_ttc = NULL;
				_checker = NULL;
				_logfile = NULL;
		};
		~CMigrateUnit()
		{
				if (_ttc) delete _ttc;
				if (_logfile) delete _logfile;
				if (_checker) delete _checker;
		};
		int DoFullMigrate(void);
		int DoIncMigrate(void);
		int Init(CMarkupSTL* xml);
	private:
		int BulkMigrate(TTC::Result& result);
		int SetTTCStatus(int status);
		int DoFullMigrateInter(void);
		int DoIncMigrateInter(void);
	private:
		CRequest * _ttc;
		CKeyChecker * _checker;
		int _step;
		std::set<std::string> _filter;
		std::set<std::string> _serverlist;
		CFile * _logfile;
};

#endif
