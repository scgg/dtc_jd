#ifndef __STAT_TTC_DEFINITION__
#define __STAT_TTC_DEFINITION__

#include "StatInfo.h"

enum {
	//相同的统计项目
	S_VERSION = 5,
	C_TIME,

	LOG_COUNT_0 = 10,
	LOG_COUNT_1,
	LOG_COUNT_2,
	LOG_COUNT_3,
	LOG_COUNT_4,
	LOG_COUNT_5,
	LOG_COUNT_6,
	LOG_COUNT_7,

	REQ_USEC_ALL = 20,
	REQ_USEC_GET,
	REQ_USEC_INS,
	REQ_USEC_UPD,
	REQ_USEC_DEL,
	REQ_USEC_FLUSH,
	REQ_USEC_HIT,
	REQ_USEC_REPLACE,

	ACCEPT_COUNT = 30, 		// accept连接次数
	CONN_COUNT, 			// 当前连接数

	CUR_QUEUE_COUNT,
	MAX_QUEUE_COUNT,
	QUEUE_EFF,

	AGENT_ACCEPT_COUNT,
	AGENT_CONN_COUNT,

	SERVER_READONLY = 40,  		//server是否为只读状态
	SERVER_OPENNING_FD,
	SUPER_GROUP_ENABLE,

	//TTC
	TTC_CACHE_SIZE = 1000,
	TTC_CACHE_KEY,
	TTC_CACHE_VERSION,
	TTC_UPDATE_MODE,
	TTC_EMPTY_FILTER,

	TTC_USED_NGS,
	TTC_USED_NODES,
	TTC_DIRTY_NODES,
	TTC_USED_ROWS,
	TTC_DIRTY_ROWS,
	TTC_BUCKET_TOTAL,
	TTC_FREE_BUCKET,
	TTC_DIRTY_AGE,
	TTC_DIRTY_ELDEST,

	TTC_CHUNK_TOTAL,
	TTC_DATA_SIZE,
	TTC_DATA_EFF,

	TTC_DROP_COUNT,
	TTC_DROP_ROWS,
	TTC_FLUSH_COUNT,
	TTC_FLUSH_ROWS,
	TTC_GET_COUNT,
	TTC_GET_HITS,
	TTC_INSERT_COUNT,
	TTC_INSERT_HITS,
	TTC_UPDATE_COUNT,
	TTC_UPDATE_HITS,
	TTC_DELETE_COUNT,
	TTC_DELETE_HITS,
	TTC_PURGE_COUNT,
	TTC_HIT_RATIO,
	TTC_ASYNC_RATIO,

	TTC_SQL_USEC_ALL,
	TTC_SQL_USEC_GET,
	TTC_SQL_USEC_INS,
	TTC_SQL_USEC_UPD,
	TTC_SQL_USEC_DEL,
	TTC_SQL_USEC_FLUSH,

    TTC_FORWARD_USEC_ALL,
    TTC_FORWARD_USEC_GET,
    TTC_FORWARD_USEC_INS,
    TTC_FORWARD_USEC_UPD,
    TTC_FORWARD_USEC_DEL,
    TTC_FORWARD_USEC_FLUSH,
	
	DTC_FRONT_BARRIER_COUNT,
	DTC_FRONT_BARRIER_MAX_TASK,
	DTC_BACK_BARRIER_COUNT,
	DTC_BACK_BARRIER_MAX_TASK,

	TTC_EMPTY_NODES, // empty node count
	TTC_MEMORY_TOP,
	TTC_MAX_FLUSH_REQ,
	TTC_CURR_FLUSH_REQ,
	TTC_OLDEST_DIRTY_TIME,
    LAST_PURGE_NODE_MOD_TIME,
    DATA_EXIST_TIME,
    DATA_SIZE_AVG_RECENT,
    TTC_ASYNC_FLUSH_COUNT,


	BTM_INDEX_1 = 2000,
	BTM_INDEX_2,
	BTM_INDEX_3,
	BTM_DATA,
	BTM_DATA_DELETE,

	/* statisitc item for hotbackup */
	HBP_LRU_SCAN_TM = 3000,
	HBP_LRU_TOTAL_BITS,
	HBP_LRU_TOTAL_1_BITS,
	HBP_LRU_SET_COUNT,
	HBP_LRU_SET_HIT_COUNT,
	HBP_LRU_CLR_COUNT,
	HBP_INC_SYNC_STEP,

	/* statistic item for blacklist */
	BLACKLIST_CURRENT_SLOT = 3010,
	BLACKLIST_SIZE,
	TRY_PURGE_COUNT, /* TryPurgeSize 每次purge的节点个数 */
	TRY_PURGE_NODES,

	PLUGIN_REQ_USEC_ALL = 10000,

	/*thread cpu statistic*/
	INC_THREAD_CPU_STAT_0 = 20000,
	INC_THREAD_CPU_STAT_1,
	INC_THREAD_CPU_STAT_2,
	INC_THREAD_CPU_STAT_3,
	INC_THREAD_CPU_STAT_4,
	INC_THREAD_CPU_STAT_5,
	INC_THREAD_CPU_STAT_6,
	INC_THREAD_CPU_STAT_7,
	INC_THREAD_CPU_STAT_8,
	INC_THREAD_CPU_STAT_9,

	CACHE_THREAD_CPU_STAT = 20100,
	DATA_SOURCE_CPU_STAT = 20200,
	
	DATA_SIZE_HISTORY_STAT = 20300,
	ROW_SIZE_HISTORY_STAT = 20301,
	DATA_SURVIVAL_HOUR_STAT = 20302,
	/*新增Helper的统计项, 从20400开始编号*/
	HELPER_READ_GROUR_CUR_QUEUE_MAX_SIZE = 20400,
	HELPER_WRITE_GROUR_CUR_QUEUE_MAX_SIZE = 20401,
	HELPER_COMMIT_GROUR_CUR_QUEUE_MAX_SIZE = 20402,
	HELPER_SLAVE_READ_GROUR_CUR_QUEUE_MAX_SIZE = 20403,

};

extern TStatDefinition StatDefinition[];
#endif
