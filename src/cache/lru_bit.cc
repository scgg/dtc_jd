#include "lru_bit.h"
#include "memcheck.h"
#include "admin_tdef.h"
#include "memcheck.h"
#include "field.h"
#include "tabledef.h"
#include "node.h"
#include "node_index.h"
#include "log.h"

CLruBitObj::CLruBitObj(CLruBitUnit *t):
	_max_lru_bit(0),
	_scan_lru_bit(0),
	_scan_idx_off(0),
	_lru_writer(0),
	_owner(t),
	scan_tm(0)

{
	bzero(_lru_bits, LRU_BITS*sizeof(lru_bit_t *));

	lru_scan_tm 	= statmgr.GetItemU32(HBP_LRU_SCAN_TM);
	total_bits    	= statmgr.GetItemU32(HBP_LRU_TOTAL_BITS);
	total_1_bits  	= statmgr.GetItemU32(HBP_LRU_TOTAL_1_BITS);
	lru_set_count 	= statmgr.GetItemU32(HBP_LRU_SET_COUNT);
	lru_clr_count 	= statmgr.GetItemU32(HBP_LRU_CLR_COUNT);
	lru_set_hit_count = statmgr.GetItemU32(HBP_LRU_SET_HIT_COUNT);
}

CLruBitObj::~CLruBitObj()
{
	for(int i = 0; i < LRU_BITS; i++)
	{
		DELETE(_lru_bits[i]);
	}
}

int CLruBitObj::SetNodeID(unsigned int v, int b)
{
	int off = BBLK_OFF(v);

	if(!_lru_bits[off])
	{
		NEW(lru_bit_t, _lru_bits[off]);
		if(!_lru_bits[off])
			return -1;
	}

	/* stat set/clr count */
	if(b) 
		lru_set_count++;
	else 
		lru_clr_count++;


	if(_lru_bits[off]->set(v, b)) {
		/* stat set hit count */
		lru_set_hit_count++;
	}
	else if(b){
		total_1_bits++;
	}

	_max_lru_bit < off ? _max_lru_bit = off : off;


	/* stat total bits */
	total_bits < v ? total_bits=v : total_bits;

	return 0;
}

void CLruBitObj::TimerNotify(void)
{

	Scan();

	AttachTimer(_owner->_scan_timerlist);
}

int CLruBitObj::Init(CBinlogWriter *w, int stop_until)
{
	_scan_stop_until = stop_until;

	NEW(CLruWriter(w), _lru_writer);
	if(!_lru_writer)
		return -1;

	if(_lru_writer->Init())
		return -1;

	return 0;
}

int CLruBitObj::Scan(void)
{
	if(scan_tm == 0){
		INIT_MSEC(scan_tm);
	}

	lru_bit_t *p = _lru_bits[_scan_lru_bit];
	if(!p)
		return 0;

	unsigned found_id = 0;
	for(; _scan_idx_off < IDX_SIZE;)
	{
		unsigned found = 0;

		//扫描idx中的1 byte, 最大会有512个node id
		for(int j = 0; j < 8; ++j)
		{
			//读取idx中的第_scan_idx_off个字节的第j位对应的blk中的8 bytes
			uint64_t v = p->read(_scan_idx_off, j);
			if(0 == v)
				continue;

			//扫描blk中的8 bytes
			for(int i = 0; i < 64; ++i)
			{
				if(v&0x1)
				{
					found += 1;

					uint32_t id = (_scan_lru_bit << 21) + (_scan_idx_off << 9) + (j << 6) + i;

					log_debug("adjust lru: node-id=%u", id);

					_lru_writer->Write(id);
				}

				v >>= 1;
			}
		}

		if(found>0)
		{
			//批量写入lru变更
			_lru_writer->Commit();

			//idx清零1byete， blk清零64bytes
			total_1_bits -= p->clear(_scan_idx_off);
		}

		_scan_idx_off += 1;

		found_id += found;
		// 如果超过此水位，终止扫描, 等待下一次被调度
		if(found_id >= _scan_stop_until)
		{
			return 0;
		}
	}

	//调整为下一个lru_bit(4k)
	_scan_idx_off = 0;
	_scan_lru_bit += 1;
	if(_scan_lru_bit > _max_lru_bit) {

		_scan_lru_bit = 0;

		CALC_MSEC(scan_tm);
		lru_scan_tm = scan_tm;
		scan_tm = 0;
	}

	return 0;
}

CLruBitUnit::CLruBitUnit(CTimerUnit *p):
	_scan_timerlist(0),
	_lru_bit_obj(0),
	_is_start(0),
	_owner(p)
{
}

CLruBitUnit::~CLruBitUnit()
{
	DELETE(_lru_bit_obj);
}

int CLruBitUnit::Init(CBinlogWriter *w)
{
	_scan_timerlist = _owner->GetTimerListByMSeconds(LRU_SCAN_INTERVAL);

	NEW(CLruBitObj(this), _lru_bit_obj);
	if(!_lru_bit_obj)
		return -1;

	if(_lru_bit_obj->Init(w))
		return -1;

	return 0;
}

void CLruBitUnit::EnableLog(void)
{
	_is_start=1;
	_lru_bit_obj->AttachTimer(_scan_timerlist);
}

void CLruBitUnit::DisableLog(void)
{
	_is_start = 0;
	_lru_bit_obj->DisableTimer();
}

int CLruBitUnit::Set(unsigned int v)
{
	return _is_start ? _lru_bit_obj->SetNodeID(v, 1):0;
}

int CLruBitUnit::Unset(unsigned int v)
{
	return _is_start ? _lru_bit_obj->SetNodeID(v, 0):0;
}


extern CTableDefinition *gTableDef[];

CLruWriter::CLruWriter(CBinlogWriter *w):
	_log_writer(w),
	_raw_data(0)
{
}       

CLruWriter::~CLruWriter()
{
	DELETE(_raw_data);
}

int CLruWriter::Init()
{
	NEW(CRawData(&g_stSysMalloc, 1), _raw_data);
	if(!_raw_data)
		return -1;

	unsigned type = CHotBackup::SYNC_LRU;
	if(_raw_data->Init(0, gTableDef[1]->KeySize(), (const char *)&type))
		return -1;

	return 0;
}

int CLruWriter::Write(unsigned int v)
{
	log_debug("enter CLruWriter, lru changes, node id:%u", v);

	CNode node = I_SEARCH(v);
	if(!node) //NODE已经不存在，不处理 
		return 0;

	CDataChunk *p = M_POINTER(CDataChunk, node.VDHandle());
	CRowValue r(gTableDef[1]);

	r[0].u64 = CHotBackup::SYNC_LRU;
	r[1].u64 = CHotBackup::NON_VALUE;

	//self table-definition encode packed key
	r[2] = gTableDef[0]->PackedKey(p->Key());
	r[3].Set(0);

	return _raw_data->InsertRow(r, false, false);
}

int CLruWriter::Commit(void)
{
	log_debug("lru write commit");

	_log_writer->InsertHeader(BINLOG_LRU, 0, 1);
	_log_writer->AppendBody(_raw_data->GetAddr(), _raw_data->DataSize());

	log_debug("body: len=%d, content:%x", _raw_data->DataSize(), *(char *)_raw_data->GetAddr());

	_raw_data->DeleteAllRows();
	return _log_writer->Commit();
}
