#ifndef CACHE_MEMORY_INFO_H
#define CACHE_MEMORY_INFO_H
#include "config.h"
#include "dbconfig.h"
#include "cache_pool.h"
#include "raw_data_process.h"

class CCacheMemoryInfo : public CCachePool
{
private:
	CConfig* m_stConfig;
	CDbConfig* m_stDbConfig;
	CTableDefinition* m_stTableDef;
	uint64_t m_RelativeHour;

	CNode m_stCurNode;
	CRawFormat* m_pstRawFormatItem;
public:
	CCacheMemoryInfo();
	~CCacheMemoryInfo();
	int cache_open();
	int get_cout();
	int last_acc();
	int last_upd();
	int survival();
	//CRawData* get_cur_rawdata();
	int data_size();
	int row_size();
	char* key();
	int total_dirty_node();
	int total_used_node();
	bool set_node(CNode &node);

};

#endif

