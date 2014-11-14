/*
 * =====================================================================================
 *
 *       Filename:  defragment.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/19/2010 03:22:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "bin_malloc.h"
#include "ttcapi.h"

class CDefragment
{
    public:
        CDefragment()
        {
            _mem = NULL;
            _pstChunk = NULL;        
            _keysize = -1;
            _s = NULL;
            _error_count = 0;
            _skip_count = 0;
            _ok_count = 0;
			_bulk_per_ten_microscoends = 1;
        }

        ~CDefragment()
        {

	}
	int Attach(const char * key,int keysize, int step);
	char * GetKeyByHandle(INTER_HANDLE_T handle,int *len);
	int Proccess(INTER_HANDLE_T handle);
	int DumpMem(bool verbose = false);
	int DumpMemNew(const char * filename,uint64_t& memsize);
	int DefragMem(int level,TTC::Server *s);
	int DefragMemNew(int level,TTC::Server* s,const char *filename,uint64_t memsize);
	int ProccessHandle(const char * filename,TTC::Server* s);
	void frequency_limit(void);

    private:
        CBinMalloc * _mem;
        CMallocChunk* _pstChunk;
        int _keysize;
        TTC::Server* _s;

        //stat
        uint64_t _error_count;
        uint64_t _skip_count;
        uint64_t _ok_count;
		int _bulk_per_ten_microscoends;
};

#define SEARCH 0
#define MATCH 1
class CDefragMemAlgo
{
    public:
        CDefragMemAlgo(int level,CDefragment * master);
        ~CDefragMemAlgo();
        int Push(INTER_HANDLE_T handle,int used);
    private:
        int _status;
        INTER_HANDLE_T * _queue;
        CDefragment * _master;
        int _count;
        int _level;
};

