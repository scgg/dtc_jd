/*
 * =====================================================================================
 *
 *       Filename:  request.h
 *
 *    Description:  本对象用于向TTC发送各种命令
 *
 *        Version:  1.0
 *        Created:  11/20/2010 11:22:25 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _MIGRATE_ANGET_REQUEST_H_
#define _MIGRATE_ANGET_REQUEST_H_
#include "ttcapi.h"
#include "journal_id.h"

enum MigrationState
{
    MS_NOT_STARTED,
    MS_MIGRATING,
    MS_MIGRATE_READONLY,
    MS_COMPLETED,
};

class CRequest
{
		public:
				CRequest(const char* addr, int timeout, int count_per_second = 5000, int error_count = 3)
				{
						int to = timeout<1?1:timeout;
						_server.StringKey();
						_server.SetTableName("*");
						_server.SetAddress(addr);
						_server.SetTimeout(timeout);
						_error_count = error_count;
						_bulk_per_ten_microscoends = count_per_second/100;
						if (_bulk_per_ten_microscoends < 1) _bulk_per_ten_microscoends = 1;
				};
				~CRequest()
				{
				};
				int MigrateOneKey(const char *key,const int len);
				int Regist(CJournalID& jid);
				int GetKeyList(int limit,int step,TTC::Result& result);
				int GetUpdateKeyList(int step,CJournalID jid,TTC::Result& result);
				int SetStatus(const char * name, int state);
				void frequency_limit(void);
		private:
				TTC::Server _server;
				int _error_count;//Set命令的重试次数
				int _bulk_per_ten_microscoends;
};
#endif
