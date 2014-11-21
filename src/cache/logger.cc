#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <strings.h>
#include "logger.h"
#include "log.h"
#include "global.h"

///////////////// CLogBase
CLogBase::CLogBase():
	_fd(-1)
{
	bzero(_path, sizeof(_path));
	bzero(_prefix, sizeof(_prefix));
}

CLogBase::~CLogBase()
{
	close_file();
}

int CLogBase::set_path(const char *path, const char *prefix)
{
	snprintf(_path, sizeof(_path), "%s", path);
	snprintf(_prefix, sizeof(_prefix), "%s", prefix);

	mkdir(_path, 0777);

	if(access(_path, W_OK|X_OK) < 0)
	{
		log_error("dir(%s) Not writable", _path);
		return -1;
	}

	return 0;
}

void CLogBase::close_file()
{
	if(_fd > 0)
	{
		::close(_fd);
		_fd = -1;
	}
}

int CLogBase::stat_size(off_t *s)
{
	struct stat st;
	if(fstat(_fd, &st))
		return -1;

	*s = st.st_size;
	return 0;
}

int CLogBase::delete_file(uint32_t serial)
{
	char file[MAX_PATH_NAME_LEN] = {0};
	file_name(file, MAX_PATH_NAME_LEN, serial);

	return unlink(file);
}

int CLogBase::open_file(uint32_t serial, int read)
{
	char file[MAX_PATH_NAME_LEN] = {0};
	file_name(file, MAX_PATH_NAME_LEN, serial);

	read ? _fd = ::open(file, O_RDONLY|O_LARGEFILE, 0644) : 
		_fd = ::open(file, O_WRONLY|O_APPEND|O_CREAT|O_LARGEFILE|O_TRUNC, 0644);

	if(_fd < 0)
	{
		log_debug("open file[%s] failed, %m", file);
		return -1;
	}

	return 0;
}

int CLogBase::scan_serial(uint32_t *min, uint32_t *max)
{
	DIR *dir = opendir(_path);
	if(!dir)
		return -1;

	struct dirent *drt = readdir(dir);
	if(!drt){
		closedir(dir);
		return -2;
	}

	*min = (uint32_t)((1ULL<<32)-1);
	*max = 0;

	char prefix[MAX_PATH_NAME_LEN] = {0};
	snprintf(prefix, MAX_PATH_NAME_LEN, "%s.binlog.", _prefix);

	int l = strlen(prefix);
	uint32_t v =0;
	int found = 0;

	for(;drt;drt=readdir(dir))
	{
		int n = strncmp(drt->d_name, prefix, l);
		if(n == 0)
		{
			v = strtoul(drt->d_name+l, NULL, 10);
			v >= 1 ? (*max < v ? *max=v:v),(v < *min ? *min=v:v):v;
			found = 1;
		}
	}

	found ? *max: (*max=0,*min=0);

	log_debug("scan serial: min=%u, max=%u\n", *min, *max);

	closedir(dir);
	return 0;
}

void CLogBase::file_name(char *s, int len, unsigned serial)
{
	snprintf(s, len, "%s/%s.binlog.%u", _path, _prefix, serial);
}


///////////////// CLogWriter
CLogWriter::CLogWriter():
	CLogBase(),
	_cur_size(0),
	_max_size(0),
	_total_size(0),
	_cur_max_serial(0), //serial start 0
	_cur_min_serial(0)  //serial start 0
{
}

CLogWriter::~CLogWriter()
{
}

int CLogWriter::open(const char *path, const char *prefix, 
		off_t max_size, uint64_t total_size)
{
	if(set_path(path, prefix))
		return -1;

	_max_size   = max_size;
	_total_size = total_size;

	if(scan_serial(&_cur_min_serial, &_cur_max_serial))
	{
		log_debug("scan file serial failed, %m");
		return -1;
	}

	_cur_max_serial += 1; //skip current binlog file.
	return open_file(_cur_max_serial, 0);
}

int CLogWriter::write(const void *buf, size_t size)
{
	int unused;

	unused = ::write(_fd, buf, size);

	_cur_size += size;
	return shift_file();
}

CJournalID CLogWriter::query()
{
	CJournalID v(_cur_max_serial, _cur_size);
	return v;
}

int CLogWriter::shift_file()
{
	int need_shift  = 0;
	int need_delete = 0;

	if(_cur_size >= _max_size )
		need_shift = 1;
	else
		return 0;


	uint64_t total = _cur_max_serial - _cur_min_serial;
	total *= _max_size;

	if(total >= _total_size)
	{
		need_delete = 1;
	}


	log_debug("shift file: cur_size:"UINT64FMT", total_size:"UINT64FMT",  \
			shift:%d, cur_min_serial=%u, cur_max_serial=%u\n", 
			total, _total_size, need_shift, _cur_min_serial, _cur_max_serial);

	if(need_shift)
	{
		if(need_delete)
		{
			delete_file(_cur_min_serial);
			_cur_min_serial += 1;
		}

		close_file();

		_cur_size = 0;
		_cur_max_serial += 1;
	}

	return open_file(_cur_max_serial, 0);
}


////////////////// CLogReader
CLogReader::CLogReader():
	CLogBase(),
	_min_serial(0),
	_max_serial(0),
	_cur_serial(0),
	_cur_offset(0)
{
}

CLogReader::~CLogReader()
{
}

int CLogReader::open(const char *path, const char *prefix)
{
	if(set_path(path, prefix))
		return -1;

	//refresh directory
	refresh();

	_cur_serial = _min_serial;
	_cur_offset = 0;

	return open_file(_cur_serial, 1);
}

/*
 *  BUG
 *
 遇到最后一个日志文件，轮询文件大小(文件大小可能随时改变)
 int CLogReader::endof(void)
 {
 off_t max_offset = 0;

 if(_cur_serial == _max_serial)
 {
 stat_size(&max_offset);
 if(_cur_offset == max_offset) 
 return 1;
 }

 return 0;
 }
 */

void CLogReader::refresh()
{
	scan_serial(&_min_serial, &_max_serial);
}

int CLogReader::read(void *buf, size_t size)
{
	ssize_t rd=::read(_fd, buf, size);
	if(rd == (ssize_t)size)
	{
		_cur_offset += rd;
		return 0;
	}else if(rd < 0){
		return -1;
	}

	// 如果还有更大的serial，则丢弃buf内容，切换文件。否则,回退文件指针
	refresh();

	if(_cur_serial < _max_serial)
	{
		_cur_serial += 1;
		_cur_offset  = 0;

		close_file();
		//跳过序号不存在的文件
		while(open_file(_cur_serial, 1) == -1 && _cur_serial < _max_serial)
			_cur_serial += 1;

		if(_fd > 0 && _cur_serial <= _max_serial)
			return read(buf, size);
		else
			return -1;
	}

	// 回退文件指针
	if(rd>0)
	{
		seek(CJournalID(_cur_serial, _cur_offset));
	}

	return -1;
}

CJournalID CLogReader::query()
{
	CJournalID v(_cur_serial, _cur_offset);
	return v;
}

int CLogReader::seek(const CJournalID &v)
{
	char file[MAX_PATH_NAME_LEN] = {0};
	file_name(file, MAX_PATH_NAME_LEN, v.serial);

	/* 确保文件存在 */
	if(access(file, F_OK))
		return -1;

/*
 * 上面已经做了文件存在检查,不在需要这样的判断
 */
#if 0
	if(v.serial < _min_serial || v.serial > _max_serial)
		return -1;
#endif

	if(v.serial != _cur_serial)
	{
		close_file();

		if(open_file(v.serial, 1) == -1)
		{
			log_debug("hblog %u not exist, seek failed", v.serial);
			return -1;
		}
	}

	log_debug("open serial=%u, %m", v.serial);

	off_t file_size = 0;
	stat_size(&file_size);

	if(v.offset > (uint32_t)file_size)
		return -1;

	lseek(_fd, v.offset, SEEK_SET);

	_cur_offset = v.offset;
	_cur_serial = v.serial;
	return 0;
}

/////////// CBinlogWriter
CBinlogWriter::CBinlogWriter():
	_log_writer()

{
}

CBinlogWriter::~CBinlogWriter()
{
}

int CBinlogWriter::Init(const char *path, const char *prefix, uint64_t total, off_t max_size)
{
	return _log_writer.open(path, prefix, max_size, total);
}

#define struct_sizeof(t) sizeof(((binlog_header_t*)NULL)->t)
#define struct_typeof(t) typeof(((binlog_header_t*)NULL)->t)

int CBinlogWriter::InsertHeader(uint8_t type, uint8_t operater, uint32_t count)
{
	_codec_buffer.clear();

	_codec_buffer.expand(offsetof(binlog_header_t, endof));

	_codec_buffer << (struct_typeof(length))0;				            //length
	_codec_buffer << (struct_typeof(version))BINLOG_DEFAULT_VERSION;	//version
	_codec_buffer << (struct_typeof(type))type;				            //type
	_codec_buffer << (struct_typeof(operater))operater;			        //operator
	_codec_buffer.append("\0\0\0\0\0", 5);					            //reserve char[5]
	_codec_buffer << (struct_typeof(timestamp))(time(NULL));		    //timestamp
	_codec_buffer << (struct_typeof(recordcount))count;			        //recordcount

	return 0;
}

int CBinlogWriter::AppendBody(const void *buf, size_t size)
{
	_codec_buffer.append((char *)&size, struct_sizeof(length));
	_codec_buffer.append((const char*)buf, size);

	return 0;
}


int CBinlogWriter::Commit()
{
	//计算总长度 
	uint32_t total = _codec_buffer.size();
	total -= struct_sizeof(length);

	//写入总长度
	struct_typeof(length)* length = (struct_typeof(length) *) (_codec_buffer.c_str());
	*length = total;

	return _log_writer.write(_codec_buffer.c_str(), _codec_buffer.size());
}

//TODO: 暂时只abort掉codec_buffer, 以后可以考虑abort掉上次写入的记录
int CBinlogWriter::Abort()
{
	_codec_buffer.clear();
	return 0;
}

CJournalID CBinlogWriter::QueryID()
{
	return _log_writer.query();
}

///////////////// CBinlogReader
CBinlogReader::CBinlogReader():
	_log_reader()
{
}
CBinlogReader::~CBinlogReader()
{
}

int CBinlogReader::Init(const char *path, const char *prefix)
{
	return _log_reader.open(path, prefix);
}

int CBinlogReader::Read()
{
	/* prepare buffer */
	if(_codec_buffer.resize(struct_sizeof(length)) < 0)
	{
		log_error("expand _codec_buffer failed");
		return -1;
	}
	/* read length part of one binlog */
	if(_log_reader.read(_codec_buffer.c_str(), struct_sizeof(length)))
		return -1;

	struct_typeof(length) len = *(struct_typeof(length) *)_codec_buffer.c_str();
	if(len < 8 || len >= (1<<20)/*1M*/)
	{
		// filter some out of range length,
		// prevent client sending invalid jid crash server
		return -1;
	}
	_codec_buffer.resize(len+struct_sizeof(length));
	if(_log_reader.read(_codec_buffer.c_str()+struct_sizeof(length), len))
		return -1;

	return 0;
}

CJournalID CBinlogReader::QueryID()
{
	return _log_reader.query();
}

int CBinlogReader::Seek(const CJournalID &v)
{
	return _log_reader.seek(v);
}

uint8_t CBinlogReader::BinlogType()
{
	return ((binlog_header_t *)(_codec_buffer.c_str()))->type;
}

uint8_t CBinlogReader::BinlogOperator()
{
	return ((binlog_header_t *)(_codec_buffer.c_str()))->operater;
}

uint32_t CBinlogReader::RecordCount()
{
	return ((binlog_header_t *)(_codec_buffer.c_str()))->recordcount;
}

/*
 * binlog format:
 *
 * =====================================================
 *  binlog_header_t | len1 | record1 | len2 | record2 | ...
 * =====================================================
 *
 */
char * CBinlogReader::RecordPointer(int id) 
{
	//record start
	char *p = (char *)(_codec_buffer.c_str() + offsetof(binlog_header_t, endof));
	char *m = 0;
	uint32_t l = struct_sizeof(length);
	uint32_t ll = 0;

	for(int i = 0; i<=id;i++)
	{
		m  = p + l; 
		ll = *(struct_typeof(length) *)(m-struct_sizeof(length));
		l  += (ll+struct_sizeof(length));
	}

	return m;
}

size_t CBinlogReader::RecordLength(int id)
{
	char *p = (char *)(_codec_buffer.c_str() + offsetof(binlog_header_t, endof));
	uint32_t ll, l;
	l = ll = 0;

	for(int i = 0; i<=id; i++)
	{
		l = *(struct_typeof(length) *)(p+ll);
		ll += (l+struct_sizeof(length));
	}

	return l;
}
