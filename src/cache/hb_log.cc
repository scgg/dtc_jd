#include "hb_log.h"
#include "global.h"

CHBLog::CHBLog(CTableDefinition *tbl) :
	_enable(false),
	_tabledef(tbl),
	_log_writer(0),
	_log_reader(0)
{
}

CHBLog::~CHBLog()
{
	DELETE(_log_writer);
	DELETE(_log_reader);
}

int CHBLog::Init(const char *path, const char *prefix)
{
	_log_writer = new CBinlogWriter;
	_log_reader = new CBinlogReader;

	if(_log_writer->Init(path, prefix))
	{
		log_error("init log_writer failed");
		return -1;
	}


	if(_log_reader->Init(path, prefix))
	{
		log_error("init log_reader failed");
		return -2;
	}

	return 0;
}

void CHBLog::EnableLog(void)
{
	_enable = true;
}

void CHBLog::DisableLog(void)
{
	_enable = false;
}

int CHBLog::WriteUpdateKey(CValue key, CValue value, int type)
{
	//没有开启日志记录，直接返回
	if(!_enable)
		return 0;

	log_debug("HBLog write update key, key:%u", *(unsigned *)key.bin.ptr);

	CRawData *raw_data = NULL;
	NEW(CRawData(&g_stSysMalloc, 1), raw_data);

	if(!raw_data)
		return -1;

	if(raw_data->Init(0, _tabledef->KeySize(), (const char *)&type))
	{
		DELETE(raw_data);
		return -1;
	}

	CRowValue r(_tabledef);
	r[0].u64 = type;
	r[1].u64 = CHotBackup::HAS_VALUE;
	r[2] = key;
	r[3] = value;

	raw_data->InsertRow(r, false, false);

	_log_writer->InsertHeader(type, 0, 1);
	_log_writer->AppendBody(raw_data->GetAddr(), raw_data->DataSize());

	DELETE(raw_data);

	log_debug("type:"UINT64FMT", flag: "UINT64FMT, r[0].u64, r[1].u64);

	return _log_writer->Commit();
}

int CHBLog::WriteUpdateKey(CValue key, int type)
{
	//没有开启日志记录，直接返回
	if(!_enable)
		return 0;

	log_debug("HBLog write update key without value, key:%u", *(unsigned *)key.bin.ptr);

	CRawData *raw_data;
	NEW(CRawData(&g_stSysMalloc, 1), raw_data);

	if(!raw_data)
		return -1;

	if(raw_data->Init(0, _tabledef->KeySize(), (const char *)&type))
	{
		DELETE(raw_data);
		return -1;
	}

	CRowValue r(_tabledef);
	r[0].u64 = type;
	r[1].u64 = CHotBackup::NON_VALUE;
	r[2] = key;
	r[3].Set(0);

	raw_data->InsertRow(r, false, false);

	_log_writer->InsertHeader(type, 0, 1);
	_log_writer->AppendBody(raw_data->GetAddr(), raw_data->DataSize());

	log_debug("type: "UINT64FMT", flag: "UINT64FMT, r[0].u64, r[1].u64);


	DELETE(raw_data);

	return _log_writer->Commit();
}

int CHBLog::Seek(const CJournalID &v)
{
	return _log_reader->Seek(v);
}

/* 批量拉取更新key，返回更新key的个数 */
int CHBLog::TaskAppendAllRows(CTaskRequest &task, int limit)
{
	int count;
	for(count=0; count<limit; ++count)
	{
		/* 没有待处理日志 */
		if(_log_reader->Read())
			break;

		CRawData *raw_data;

		NEW(CRawData(&g_stSysMalloc, 0), raw_data);

		if(!raw_data) {
			log_error("allocate rawdata mem failed");
			return -1;
		}

		if(raw_data->CheckSize(g_stSysMalloc.Handle(_log_reader->RecordPointer()), 
					0, 
					_tabledef->KeySize(), 
					_log_reader->RecordLength(0)) < 0)
		{
			log_error("raw data broken: wrong size");
			DELETE(raw_data);
			return -1;
		}

               /* attach raw data read from one binlog */
		if(raw_data->Attach(g_stSysMalloc.Handle(_log_reader->RecordPointer()), 0, _tabledef->KeySize())) {
			log_error("attach rawdata mem failed");

			DELETE(raw_data);
			return -1;
		}

		CRowValue r(_tabledef);
		r[0].u64 = *(unsigned *)raw_data->Key();


		unsigned char flag=0;
		while(raw_data->DecodeRow(r, flag) == 0) {

			log_debug("type: "UINT64FMT", flag: "UINT64FMT", key:%u",
				       	r[0].u64, r[1].u64, *(unsigned *)r[2].bin.ptr);
			log_debug("binlog-type: %d", _log_reader->BinlogType());

			task.AppendRow(&r);
		}

		DELETE(raw_data);
	}

	return count;
}

CJournalID CHBLog::GetReaderJID(void)
{
	return _log_reader->QueryID();
}

CJournalID CHBLog::GetWriterJID(void)
{
	return _log_writer->QueryID();
}
