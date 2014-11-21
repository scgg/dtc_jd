#ifndef __CACHE_READER_H
#define __CACHE_READER_H

#include "reader_interface.h"
#include "cache_pool.h"
#include "tabledef.h"
#include "raw_data_process.h"

class cCacheReader : public cReaderInterface, public CCachePool
{
	private:
		CNode stClrHead;
		CNode stDirtyHead;
		int iInDirtyLRU;
		CNode stCurNode;
		unsigned char uchRowFlags;
		CRawData* pstItem;
		CDataProcess* pstDataProcess;
		int notFetch;
		int curRowIdx;
		char error_message[200];
		
	public:
		cCacheReader(void);
		~cCacheReader(void);

		int cache_open(int shmKey, int keySize, CTableDefinition* pstTab);
		
		const char* err_msg(){ return error_message; }
		int begin_read();
		int read_row(CRowValue& row);
		int end();
		int key_flags(void) const { return stCurNode.IsDirty(); }
		int key_flag_dirty(void) const { return stCurNode.IsDirty(); }
		int row_flags(void) const { return uchRowFlags; }
		int row_flag_dirty(void) const { return uchRowFlags & OPER_DIRTY; }
		int fetch_node();
		int num_rows();
};

inline int cCacheReader::end()
{
	return (iInDirtyLRU==0) && (notFetch==0) && (stCurNode==stClrHead);
}


#endif
