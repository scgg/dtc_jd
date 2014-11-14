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

	CNode m_stCleanHead;
	CNode m_stDirtyHead;
	CNode m_stEmptyHead;
	CNode m_stCurNode;
	CRawData* m_pstItem;
	CRawFormat* m_pstRawFormatItem;
	CDataProcess* m_pstDataProcess;
public:
	CCacheMemoryInfo();
	~CCacheMemoryInfo();
	int cache_open();
	int begin_clean();
	int begin_dirty();
	int begin_empty();
	int fetch_node();
	int get_cout();
	int last_acc();
	int last_upd();
	//CRawData* get_cur_rawdata();
	int data_size();
	int row_size();
	char* key();
	int total_dirty_node();
	int total_used_node();
	bool end();

};

#endif

