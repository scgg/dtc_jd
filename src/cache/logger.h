/* $Id: logger.h 7428 2010-10-29 09:26:45Z foryzhou $ */
/* ttc binlog implementation.*/ 

#ifndef __TTC_LOGGER_H
#define __TTC_LOGGER_H

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "buffer.h"
#include "log.h"
#include "journal_id.h"

#define MAX_PATH_NAME_LEN	256

/*
 * TTC binlog base class(file)
 */
class CLogBase
{
	public:
		CLogBase();
		virtual ~CLogBase();

	protected:
		int set_path(const char *path, const char *prefix);
		void file_name(char *s, int len, uint32_t serail);
		int  open_file(uint32_t serial, int read);
		void close_file();
		int  scan_serial(uint32_t *min, uint32_t *max);
		int  stat_size(off_t *);
		int  delete_file(uint32_t serial);

	private:
		CLogBase(const CLogBase&);

	protected:
		int 		_fd;
	private:
		char 		_path[MAX_PATH_NAME_LEN];	//日志集所在目录
		char 		_prefix[MAX_PATH_NAME_LEN];	//日志集的文件前缀
};

class CLogWriter : public CLogBase
{
	public:
		int  open(const char *path, const char *prefix, 
				off_t max_size, uint64_t total_size);
		int  write(const void *buf, size_t size);
		CJournalID query();

	public:
		CLogWriter();
		virtual ~CLogWriter();

	private:
		int  shift_file();
	private:
		off_t 		_cur_size;			//当前日志文件的大小
		off_t 		_max_size;			//单个日志文件允许的最大大小
		uint64_t 	_total_size;			//日志集允许的最大大小
		uint32_t 	_cur_max_serial;		//当前日志文件最大编号
		uint32_t 	_cur_min_serial;		//当前日志文件最大编号
};

class CLogReader : public CLogBase
{
	public:
		int open(const char *path, const char *prefix);
		int read(void *buf, size_t size);

		int  seek(const CJournalID&);
		CJournalID query();
	public:
		CLogReader();
		virtual ~CLogReader();

	private:
		void refresh();
	private:
		uint32_t	_min_serial;			//日志集的最小文件编号
		uint32_t	_max_serial;			//日志集的最大文件编号
		uint32_t	_cur_serial;			//当前日志文件编号
		off_t 		_cur_offset;			//当前日志文件偏移量
};


/////////////////////////////////////////////////////////////////////
/*
 * generic binlog header
 */
typedef struct binlog_header
{
	uint32_t length;					//长度
	uint8_t  version;					//版本
	uint8_t  type;						//类型: bitmap, ttc, other
	uint8_t  operater; 					//操作: insert,select,upate ...
	uint8_t  reserve[5];					//保留
	uint32_t timestamp;					//时间戳
	uint32_t recordcount;					//子记录个数
	uint8_t  endof[0];
}__attribute__((__aligned__(1))) binlog_header_t;

/*
 * binlog type
 * t
 */
typedef enum binlog_type
{
	BINLOG_LRU    =1,
	BINLOG_INSERT =2,
	BINLOG_UPDATE =4,
	BINLOG_PRUGE  =8,

}BINLOG_TYPE;

/*
 * binlog class 
 */
#define BINLOG_MAX_SIZE   	    (100*(1U<<20))  //100M,  默认单个日志文件大小
#define BINLOG_MAX_TOTAL_SIZE       (10ULL<<30)     //10G，  默认最大日志文件编号
#define BINLOG_DEFAULT_VERSION 	    0x02

class CBinlogWriter
{
	public:
		int  Init(const char *path, const char *prefix,
				uint64_t total_size=BINLOG_MAX_TOTAL_SIZE, off_t max_size=BINLOG_MAX_SIZE);
		int  InsertHeader(uint8_t type, uint8_t operater, uint32_t recordcount);
		int  AppendBody(const void *buf, size_t size);

		int  Commit();
		int  Abort();
		CJournalID QueryID();

	public:
		CBinlogWriter();
		virtual ~CBinlogWriter();

	private:
		CBinlogWriter(const CBinlogWriter &);

	private:
		CLogWriter _log_writer;		//写者
		buffer 	   _codec_buffer;	//编码缓冲区
};

class CBinlogReader
{
	public:
		int  Init(const char *path, const char *prefix);

		int  Read();				    //顺序读，每次读出一条binlog记录
		int  Seek(const CJournalID &);
		CJournalID QueryID();

		uint8_t  BinlogType();
		uint8_t  BinlogOperator();

		uint32_t RecordCount();
		char*    RecordPointer(int id=0);
		size_t   RecordLength(int id=0);

	public:
		CBinlogReader();
		virtual ~CBinlogReader();

	private:
		CBinlogReader(const CBinlogReader&);

	private:
		CLogReader 	_log_reader;	//读者
		buffer	 	_codec_buffer;	//编码缓冲区
};

#endif
