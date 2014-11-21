#include "log.h"
#include "StatTTC.h"
extern CStatItemU32 *__statLogCount;
extern unsigned int __localStatLogCnt[8];

void _init_log_stat_(void)
{
	__statLogCount = new CStatItemU32[8];
	for(unsigned i=0; i<8; i++)
	{
		__statLogCount[i] = statmgr.GetItemU32(LOG_COUNT_0+i);
		__statLogCount[i].set(__localStatLogCnt[i]);
	}
}

// prevent memory leak if module unloaded
// unused-yet because this file never link into module
__attribute__((__constructor__))
static void clean_logstat(void) {
	if(__statLogCount != NULL) {
		delete __statLogCount;
		__statLogCount = NULL;
	}
}
