#ifndef __TTC_HB_LOG_H
#define __TTC_HB_LOG_H

#include "logger.h"
#include "journal_id.h"
#include "task_request.h"
#include "field.h"
#include "raw_data.h"
#include "admin_tdef.h"
#include "sys_malloc.h"
#include "tabledef.h"

class CBinlogWriter;
class CBinlogReader;

class CHBLog
{
    public:
        //传入编解码的表结构
        CHBLog(CTableDefinition *tbl);
        ~CHBLog();

        int  Init(const char *path, const char *prefix);

	bool LogEnabled(void) const { return _enable; }
        void EnableLog();
        void DisableLog();

        int  Seek(const CJournalID &);

        CJournalID GetReaderJID(void); 
        CJournalID GetWriterJID(void); 

        //时间不够，暂时不带value，只写更新key
        int  WriteUpdateKey(CValue key, int type);

        //将多条log记录编码进CTaskReqeust
        int  TaskAppendAllRows(CTaskRequest &, int limit); 

        //提供给CLRUBitUnit来记录lru变更
        CBinlogWriter* LogWriter(void)
        {
            return _log_writer;
        }

        int WriteUpdateKey(CValue key, CValue v, int type);

    private:
        bool    _enable;

        CTableDefinition* _tabledef;
        CBinlogWriter*    _log_writer;
        CBinlogReader*    _log_reader;
};

#endif
