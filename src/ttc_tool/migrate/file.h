/*
 * =====================================================================================
 *
 *       Filename:  comm.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/20/2010 10:45:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _MIGRATE_AGENT_FILE_H_
#define _MIGRATE_AGENT_FILE_H_
#include <string>
#include "journal_id.h"
#include "MarkupSTL.h"

typedef enum 
{
	NOT_STARTED,
	FULL_MIGRATING,
	FULL_ERROR,
	FULL_DONE,
	INC_MIGRATING,
	INC_ERROR,
	INC_DONE,
	UNKONW_STATUS,
	MAX_STATUS
} MIGRATE_STATUS;

class CFile
{
		public:
				CFile(const char * filename)
				{
						_filename = filename;
				};
				~CFile(){};
				bool Lock(void);
				std::string ReadFile(const char * name); 
				bool WriteFile(const char * name, std::string status);
				bool SetJID(CJournalID jid);
				bool GetJID(CJournalID& jid);
				bool SetStatus(MIGRATE_STATUS status);
				MIGRATE_STATUS GetStatus(void);
				bool GetXML(CMarkupSTL& xml);
		private:
				std::string _filename;
};
#endif
