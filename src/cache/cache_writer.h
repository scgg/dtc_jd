#ifndef __CACHE_WRITER_H
#define __CACHE_WRITER_H

#include "cache_pool.h"
#include "tabledef.h"
#include "writer_interface.h"
#include "raw_data_process.h"

class cCacheWriter : public cWriterInterface, public CCachePool
{
	private:
		CRawData* pstItem;
		CDataProcess* pstDataProcess;
		int iIsFull;
		int iRowIdx;
		CNode stCurNode;
		char achPackKey[MAX_KEY_LEN+1];
		char szErrMsg[200];
		
	protected:
		int AllocNode(const CRowValue& row);
		
	public:
		cCacheWriter(void);
		~cCacheWriter(void);

		int cache_open(CacheInfo* pstInfo, CTableDefinition* pstTab);
		
		const char* err_msg(){ return szErrMsg; }
		int begin_write();
		int full();
		int write_row(const CRowValue& row);
		int commit_node();
		int rollback_node();

};

#endif
