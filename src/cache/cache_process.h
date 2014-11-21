#ifndef __CACHE_RPOCESS
#define __CACHE_RPOCESS

/************************************************************
Description:    封装了初始化内存cache以及处理task请求的各种操作
Version:         TTC 3.0
Function List:
1.  cache_open()	打开cache内存
2.  cache_process_request()	处理task请求
3. cache_process_reply() 		处理helper的返回
4. cache_process_error()		task出错的处理
History:
Paul    2008.07.01     3.0         build this moudle
 ***********************************************************/

#include <sys/mman.h>
#include <time.h>

#include "protocol.h"
#include "value.h"
#include "field.h"
#include "section.h"
#include "tabledef.h"
#include "task_request.h"
#include "list.h"
#include "barrier.h"
#include "cache_pool.h"
#include "poll_thread.h"
#include "dbconfig.h"
#include "lqueue.h"
#include "StatTTC.h"
#include "data_process.h"
#include "empty_filter.h"
#include "namespace.h"
#include "task_pendlist.h"
#include "data_chunk.h"
#include "hb_log.h"
#include "lru_bit.h"
#include "hb_feature.h"
#include "blacklist_unit.h"

TTC_BEGIN_NAMESPACE

class CFlushRequest;
class CCacheProcess;
class CTableDefinition;
class CTaskPendingList;
enum TCacheResult {
		CACHE_PROCESS_ERROR=-1,
		CACHE_PROCESS_OK=0,
		CACHE_PROCESS_NEXT=1,
		CACHE_PROCESS_PENDING=2,
		CACHE_PROCESS_REMOTE=3 //给远程helper。主要是migrate命令使用
};
typedef unsigned int MARKER_STAMP;

class CCacheReplyNotify: public CReplyDispatcher<CTaskRequest> {
	private:
		CCacheProcess *owner;
	public:
		CCacheReplyNotify(CCacheProcess *o) :
			owner(o)
	{}
		virtual ~CCacheReplyNotify(){}
		virtual void ReplyNotify(CTaskRequest *);
};

class CFlushReplyNotify: public CReplyDispatcher<CTaskRequest> {
	private:
		CCacheProcess *owner;
	public:
		CFlushReplyNotify(CCacheProcess *o) :
			owner(o)
	{}
		virtual ~CFlushReplyNotify(){}
		virtual void ReplyNotify(CTaskRequest *);
};

enum {
	LRU_NONE=0,
	LRU_BATCH,
	LRU_READ,
	LRU_WRITE,
	LRU_ALWAYS=999,
};

enum {
	NODESTAT_MISSING,
	NODESTAT_EMPTY,
	NODESTAT_PRESENT
};

struct CCacheTransation {
	CTaskRequest *curTask;
	const char *ptrKey;
	CNode m_stNode;
	int oldRows;
	uint8_t nodeStat;
	uint8_t keyDirty;
	uint8_t nodeEmpty;
	uint8_t lruUpdate;
	int logtype; // OLD ASYNC TRANSATION LOG
	CRawData *fstLogRows; // OLD ASYNC TRANSATION LOG
	CRawData *pstLogRows; // OLD ASYNC TRANSATION LOG

	void Init(CTaskRequest *task) {
		memset(this, 0, sizeof(CCacheTransation));
		curTask = task;
	}

	void Free(void) {
		if(fstLogRows) delete fstLogRows;
		fstLogRows = NULL;
		pstLogRows = NULL;
		logtype = 0;

		ptrKey = NULL;
		m_stNode = CNode::Empty();
		nodeStat = 0;
		keyDirty = 0;
		oldRows = 0;
		nodeEmpty = 0;
		lruUpdate = 0;
		//curTask = NULL;
	}
};

class CCacheProcess :
	public CTaskDispatcher<CTaskRequest>,
	private CTimerObject,
	public CPurgeNodeNotifier,
	public CCacheTransation
{
	protected: // base members
		// cache chain control
		CRequestOutput<CTaskRequest> output;
		CRequestOutput<CTaskRequest> remoteoutput;//将请求传给远端ttc，用于migrate命令
		CCacheReplyNotify cacheReply;

		// table info
		CTableDefinition *tableDef;
		// cache memory management
		CCachePool Cache;
		CDataProcess* pstDataProcess;
		CacheInfo cacheInfo;

		// no backup db
		bool nodbMode;
		// full cache
		bool fullMode;
		bool lossyMode;
		// treat empty key as default value, flat bitmap emulation
		bool m_bReplaceEmpty;
		// lru update level
		int noLRU;
		// working mode
		TUpdateMode asyncServer;
		TUpdateMode updateMode;
		TUpdateMode insertMode;
		/*indicate mem dirty when start with sync ttc*/
		bool mem_dirty;
		// server side sorting
		unsigned char insertOrder;

		// cache protection
		int nodeSizeLimit;  //node size limit
		int nodeRowsLimit;  //node rows limit
		int nodeEmptyLimit;  //empty nodes limit

		// generated error message
		char szErrMsg[256];

	protected: // stat subsystem
		CStatItemU32 statGetCount;
		CStatItemU32 statGetHits;
		CStatItemU32 statInsertCount;
		CStatItemU32 statInsertHits;
		CStatItemU32 statUpdateCount;
		CStatItemU32 statUpdateHits;
		CStatItemU32 statDeleteCount;
		CStatItemU32 statDeleteHits;
		CStatItemU32 statPurgeCount;

		CStatItemU32 statDropCount;
		CStatItemU32 statDropRows;
		CStatItemU32 statFlushCount;
		CStatItemU32 statFlushRows;
		CStatSample  statIncSyncStep;

		CStatItemU32 statMaxFlushReq;
		CStatItemU32 statCurrFlushReq;
		CStatItemU32 statOldestDirtyTime;
		CStatItemU32 statAsyncFlushCount;

	protected: // async flush members
		CFlushReplyNotify flushReply;
		CTimerList *flushTimer;
		volatile int nFlushReq; // current pending node
		volatile int mFlushReq; // pending node limit
		volatile unsigned short maxFlushReq; // max speed
		volatile unsigned short markerInterval;
		volatile int minDirtyTime;
		volatile int maxDirtyTime;
		// async log writer
		int noAsyncLog;


	protected:
		//空节点过滤
		CEmptyNodeFilter *m_pstEmptyNodeFilter;

	protected:
		// Hot Backup
		//记录更新key
		CHBLog      hbLog;
		//记录lru变更
		CLruBitUnit lruLog;
		CTaskPendingList taskPendList;
		CHBFeature* hbFeature;
		// Hot Backup

	protected:
		// BlackList
		CBlackListUnit *blacklist;
		CTimerList     *blacklist_timer;
		// BlackList

	private:
		// level 1 processing
		// GET entrance
		TCacheResult cache_get_data	(CTaskRequest &Task);
		// GET batch entrance
		TCacheResult cache_batch_get_data	(CTaskRequest &Task);
		// GET response, DB --> cache
		TCacheResult cache_replace_result	(CTaskRequest &Task);
		// GET response, DB --> client
		TCacheResult cache_get_rb	(CTaskRequest &Task);

		// implementation some admin/purge/flush function
		TCacheResult cache_process_admin(CTaskRequest &Task);
		TCacheResult cache_purge_data	(CTaskRequest &Task);
		TCacheResult cache_flush_data(CTaskRequest &Task);
		TCacheResult cache_flush_data_before_delete(CTaskRequest &Task);
		int cache_flush_data_timer(CNode& stNode, unsigned int& uiFlushRowsCnt);
		TCacheResult cache_flush_data(CNode& stNode, CTaskRequest* pstTask, unsigned int& uiFlushRowsCnt);

		// sync mode operation, called by reply
		TCacheResult cache_sync_insert_precheck (CTaskRequest& task);
		TCacheResult cache_sync_insert (CTaskRequest& task);
		TCacheResult cache_sync_update (CTaskRequest& task);
		TCacheResult cache_sync_replace (CTaskRequest& task);
		TCacheResult cache_sync_delete (CTaskRequest& task);

		// async mode operation, called by entrance
		TCacheResult cache_async_insert (CTaskRequest& task);
		TCacheResult cache_async_update (CTaskRequest& task);
		TCacheResult cache_async_replace (CTaskRequest& task);

		// fullcache mode operation, called by entrance
		TCacheResult cache_nodb_insert (CTaskRequest& task);
		TCacheResult cache_nodb_update (CTaskRequest& task);
		TCacheResult cache_nodb_replace (CTaskRequest& task);
		TCacheResult cache_nodb_delete (CTaskRequest& task);

		// level 2 operation
		// level 2: INSERT with async compatible, create node & clear empty filter
		TCacheResult cache_insert_row	(CTaskRequest &Task, bool async, bool setrows);
		// level 2: UPDATE with async compatible, accept empty node only if EmptyAsDefault
		TCacheResult cache_update_rows	(CTaskRequest &Task, bool async, bool setrows);
		// level 2: REPLACE with async compatible, don't allow empty node
		TCacheResult cache_replace_rows	(CTaskRequest &Task, bool async, bool setrows);
		// level 2: DELETE has no async mode, don't allow empty node
		TCacheResult cache_delete_rows	(CTaskRequest &Task);

		// very low level
		// 空结点inset default值进cache内存
		// auto clear empty filter
		TCacheResult InsertDefaultRow(CTaskRequest &Task);
		bool InsertEmptyNode(void);

		// 热备操作
		TCacheResult cache_register_hb(CTaskRequest &Task);
		TCacheResult cache_logout_hb(CTaskRequest &Task);
		TCacheResult cache_get_key_list(CTaskRequest &Task);
		TCacheResult cache_get_update_key(CTaskRequest &Task);
		TCacheResult cache_get_raw_data(CTaskRequest &Task);
		TCacheResult cache_replace_raw_data(CTaskRequest &Task);
		TCacheResult cache_adjust_lru(CTaskRequest &Task);
		TCacheResult cache_verify_hbt(CTaskRequest &Task);
		TCacheResult cache_get_hbt(CTaskRequest &Task);

		//内存整理操作
		TCacheResult cache_nodehandlechange(CTaskRequest &Task);

		//迁移操作
		TCacheResult cache_migrate(CTaskRequest &Task);

		/* we can still purge clean node if hit ratio is ok */
		TCacheResult cache_purgeforhit(CTaskRequest &Task);

		//rows限制
		TCacheResult check_allowed_insert(CTaskRequest &Task);

		TCacheResult cache_query_serverinfo(CTaskRequest &Task);


		// 热备日志
		int  WriteHBLog(const char* key, CNode& stNode, int iType);
		int  WriteHBLog(CTaskRequest &Task, CNode& stNode, int iType);
	public:
		virtual void PurgeNodeNotify(const char *key, CNode node);
		/* inc flush task stat(created by flush dirty node function) */
		void IncAsyncFlushStat() { statAsyncFlushCount++; }

	private:
		virtual void TaskNotify(CTaskRequest *);
		void ReplyNotify(CTaskRequest *);

		// flush internal
		virtual void TimerNotify(void);
		int FlushNextNode(void);
		void DeleteTailTimeMarkers();
		void GetDirtyStat();
		void CalculateFlushSpeed(int is_flush_timer);
		MARKER_STAMP CalculateCurrentMarker();

		CCacheProcess (const CCacheProcess& robj);
		CCacheProcess& operator= (const CCacheProcess& robj);

	public:
		CCacheProcess (CPollThread *, CTableDefinition *, TUpdateMode async);
		~CCacheProcess (void);

		const CTableDefinition *TableDefinition(void) const { return tableDef; }
		const char *LastErrorMessage(void) const { return szErrMsg[0] ? szErrMsg : "unknown error"; }

		void SetLimitNodeSize(int node_size) {
			nodeSizeLimit = node_size;
		}

		/* 0 =  no limit */
		void SetLimitNodeRows(int rows) {
			nodeRowsLimit = rows < 0 ? 0 : rows;
			return;
		}

		/*
		 * 0 = no limit,
		 * 1-999: invalid, use 1000 instead
		 * 1000-1G: max empty node count
		 * >1G: invalid, no limit
		 */
		void SetLimitEmptyNodes(int nodes) {
			nodeEmptyLimit = nodes <= 0 ? 0 :
				nodes < 1000 ? 1000 :
				nodes > (1<<30) ? 0 :
				nodes;
			return;
		}

		void DisableAutoPurge(void) {
			Cache.DisableTryPurge();
		}

		void SetDateExpireAlertTime(int time) {
			Cache.SetDateExpireAlertTime(time);
		}

		/*************************************************
Description:	设置cache内存大小以及版本
Input:		cacheSize	共享内存的大小
createVersion	cache内存版本号
Output:
Return:		0为成功，其他值为错误
		 *************************************************/
		int cache_set_size(unsigned long cacheSize, unsigned int createVersion);

		/*************************************************
Description:	打开共享内存并初始化
Input:		iIpcKey	共享内存的key
Output:
Return:		0为成功，其他值为错误
		 *************************************************/
		int cache_open(int iIpcKey, int iEnableEmptyFilter, int iEnableAutoDeleteDirtyShm);

		int update_mode(void) const { return updateMode; }
		int EnableNoDBMode(void);
		void EnableLossyDataSource(int v) { lossyMode = v == 0 ? false : true; }
		int DisableLRUUpdate(int);
		int DisableAsyncLog(int);

		/*************************************************
Description:	处理task请求
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_NEXT为转交helper处理，CACHE_PROCESS_PENDING为flush请求需要等待
CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_process_request(CTaskRequest &Task);

		/*************************************************
Description:	处理helper的回应
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_process_reply(CTaskRequest &Task);

		/*************************************************
Description:	处理helper的回应
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_process_batch(CTaskRequest &Task);

		/*************************************************
Description:	处理helper的回应
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_process_nodb(CTaskRequest &Task);

		/*************************************************
Description:	处理flush请求的helper回应
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_flush_reply(CTaskRequest &Task);

		/*************************************************
Description:	task出错的处理
Input:		Task	task请求
Output:
Return:		CACHE_PROCESS_OK为成功，CACHE_PROCESS_ERROR为错误
		 *************************************************/
		TCacheResult  cache_process_error(CTaskRequest &Task);

		void print_row(const CRowValue *r);
		int SetInsertOrder(int o);
		void SetReplaceEmpty(bool v) { m_bReplaceEmpty = v; }

		// stage relate
		void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
		void BindDispatcherRemote(CTaskDispatcher<CTaskRequest> *p) { remoteoutput.BindDispatcher(p); }

		// flush api
		void SetFlushParameter(int, int, int, int);
		void SetDropCount(int); // to be remove
		int CommitFlushRequest(CFlushRequest *, CTaskRequest*);
		void CompleteFlushRequest(CFlushRequest *);
		void PushFlushQueue(CTaskRequest *p) {  p->PushReplyDispatcher(&flushReply); output.IndirectNotify(p); }
		inline bool is_mem_dirty() {return mem_dirty;}
		int OldestDirtyNodeAlarm();


		friend class CTaskPendingList;
		friend class CCacheReplyNotify;

public:
	// transation implementation
		inline void TransationBegin(CTaskRequest *task) { CCacheTransation::Init(task); }
		void TransationEnd(void);
		inline int TransationFindNode(CTaskRequest &task);
		inline void TransationUpdateLRU(bool async, int type);

};


TTC_END_NAMESPACE

#endif
