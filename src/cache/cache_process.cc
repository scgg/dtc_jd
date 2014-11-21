#include <stdlib.h>
#include <stdio.h>
#include <endian.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "packet.h"
#include "log.h"
#include "cache_process.h"
#include "cache_flush.h"
#include "mysql_error.h"
#include "sys_malloc.h"
#include "data_chunk.h"
#include "raw_data_process.h"
#include "key_route.h"

TTC_USING_NAMESPACE

extern CTableDefinition* gTableDef[];
extern CKeyRoute*  keyRoute;
extern int hashChanging;
extern int targetNewHash;

inline int CCacheProcess::TransationFindNode(CTaskRequest &Task)
{
	// alreay cleared/zero-ed
	ptrKey = Task.PackedKey();
	if(m_pstEmptyNodeFilter != NULL && m_pstEmptyNodeFilter->ISSET(Task.IntKey())){
		//Cache.CachePurge(ptrKey);
		m_stNode = CNode();
		return nodeStat = NODESTAT_EMPTY;
	}

	int newhash, oldhash;
	if(hashChanging)
	{
	    if(targetNewHash)
	    {
		oldhash = 0;
		newhash = 1;
	    }else
	    {
		oldhash = 1;
		newhash = 0;
	    }

	    m_stNode = Cache.CacheFind(ptrKey, oldhash);
	    if(!m_stNode)
	    {
		m_stNode = Cache.CacheFind(ptrKey, newhash);
		if(!m_stNode)
		    return nodeStat = NODESTAT_MISSING;
	    }else
	    {
		Cache.MoveToNewHash(ptrKey, m_stNode);
	    }
	}else
	{
	    if(targetNewHash)
	    {
		m_stNode = Cache.CacheFind(ptrKey, 1);
		if(!m_stNode)
		    return nodeStat = NODESTAT_MISSING;
	    }else
	    {
		m_stNode = Cache.CacheFind(ptrKey, 0);
		if(!m_stNode)
		    return nodeStat = NODESTAT_MISSING;

	    }
	}

	keyDirty=m_stNode.IsDirty();
	oldRows = Cache.NodeRowsCount(m_stNode);
	// prepare to decrease empty node count
	nodeEmpty = keyDirty==0 && oldRows==0;
	return nodeStat = NODESTAT_PRESENT;
}

inline void CCacheProcess::TransationUpdateLRU(bool async, int level)
{
	if(!keyDirty) {
		// clear node empty here, because the lru is adjusted
		// it's not a fresh node in EmptyButInCleanList state
		if(async==true) {
			m_stNode.SetDirty();
			Cache.IncDirtyNode(1);
			Cache.RemoveFromLru(m_stNode);
			Cache.Insert2DirtyLru(m_stNode);
			if(nodeEmpty != 0) {
				// empty to non-empty
				Cache.DecEmptyNode();
				nodeEmpty = 0;
			}
			lruUpdate = LRU_NONE;
		} else {
			lruUpdate = level;
		}
	}
}

void CCacheProcess::TransationEnd(void)
{
	int newRows = 0;
	if(!!m_stNode && !keyDirty && !m_stNode.IsDirty()) {
		newRows = Cache.NodeRowsCount(m_stNode);
		int nodeEmpty1 = newRows == 0;

		if(lruUpdate > noLRU || nodeEmpty1 != nodeEmpty) {
			if(newRows == 0) {
				Cache.RemoveFromLru(m_stNode);
				Cache.Insert2EmptyLru(m_stNode);
				if(nodeEmpty == 0) {
					// non-empty to empty
					Cache.IncEmptyNode();
					nodeEmpty = 1;
				}
				//Cache.DumpEmptyNodeList();
			} else {
				Cache.RemoveFromLru(m_stNode);
				Cache.Insert2CleanLru(m_stNode);
				if(nodeEmpty != 0) {
					// empty to non-empty
					Cache.DecEmptyNode();
					nodeEmpty = 0;
				}
			}
		}
	}

	CCacheTransation::Free();
}
	

inline int  CCacheProcess::WriteHBLog(const char* key, CNode& stNode, int iType)
{
	unsigned int uiNodeSize = 0;
	CDataChunk* pstChunk = NULL;

	if(!(!stNode) && stNode.VDHandle() != INVALID_HANDLE){
		pstChunk = (CDataChunk*)CBinMalloc::Instance()->Handle2Ptr(stNode.VDHandle());
		uiNodeSize = pstChunk->NodeSize();
	}
	if(uiNodeSize > 0 && uiNodeSize <= 100){
		CValue data;
		data.Set((char*)pstChunk, uiNodeSize);
		if(hbLog.WriteUpdateKey(tableDef->PackedKey(key), data, iType))
			return -1;
	}
	else{
		if(hbLog.WriteUpdateKey(tableDef->PackedKey(key), iType))
			return -2;
	}
	// 唤醒挂起队列的所有task
	taskPendList.Wakeup();

	return 0;
}


inline int CCacheProcess::WriteHBLog(CTaskRequest &Task, CNode& stNode, int iType)
{
	return WriteHBLog(Task.PackedKey(), stNode, iType);
}

void CCacheProcess::PurgeNodeNotify(const char *key, CNode node) 
{
	if(!node)
		return;

	if(node == m_stNode) {
		if(nodeEmpty) {
			// purge an empty node! decrease empty counter
			Cache.DecEmptyNode();
			nodeEmpty = 0;
		}
		m_stNode = CNode::Empty();
	}

	if(WriteHBLog(key, node, CHotBackup::SYNC_PURGE)){
		log_crit("hb: log purge key failed");	
	}

	lruLog.Unset(node.NodeID());
}


CCacheProcess::CCacheProcess(CPollThread *p, CTableDefinition *tdef, TUpdateMode um)
	:
		CTaskDispatcher<CTaskRequest>(p),
		//	output(p, this),
		output(p),
		remoteoutput(p),
		cacheReply(this),
		tableDef(tdef),
		Cache(this),
		nodbMode(false),
		fullMode(false),
		m_bReplaceEmpty(false),
		noLRU(0),
		asyncServer(um),
		updateMode(MODE_SYNC),
		insertMode(MODE_SYNC),
		mem_dirty(false),
		insertOrder(INSERT_ORDER_LAST),
		nodeSizeLimit(0),
		nodeRowsLimit(0),
		nodeEmptyLimit(0),

		flushReply(this),
		flushTimer(NULL),
		nFlushReq(0),
		mFlushReq(0),
		maxFlushReq(1),
		markerInterval(300),
		minDirtyTime(3600),
		maxDirtyTime(43200),

		m_pstEmptyNodeFilter(NULL),
		//Hot Backup
		hbLog(gTableDef[1]),
		lruLog(p),
		taskPendList(this),
		hbFeature(NULL),
		//Hot Backup
		//BlackList
		blacklist(0),
		blacklist_timer(0)
		//BlackList
{
	memset((char *)&cacheInfo, 0, sizeof(cacheInfo));

	statGetCount	= statmgr.GetItemU32(TTC_GET_COUNT);
	statGetHits	= statmgr.GetItemU32(TTC_GET_HITS);
	statInsertCount	= statmgr.GetItemU32(TTC_INSERT_COUNT);
	statInsertHits	= statmgr.GetItemU32(TTC_INSERT_HITS);
	statUpdateCount	= statmgr.GetItemU32(TTC_UPDATE_COUNT);
	statUpdateHits	= statmgr.GetItemU32(TTC_UPDATE_HITS);
	statDeleteCount	= statmgr.GetItemU32(TTC_DELETE_COUNT);
	statDeleteHits	= statmgr.GetItemU32(TTC_DELETE_HITS);
	statPurgeCount	= statmgr.GetItemU32(TTC_PURGE_COUNT);

	statDropCount	= statmgr.GetItemU32(TTC_DROP_COUNT);
	statDropRows	= statmgr.GetItemU32(TTC_DROP_ROWS);
	statFlushCount	= statmgr.GetItemU32(TTC_FLUSH_COUNT);
	statFlushRows	= statmgr.GetItemU32(TTC_FLUSH_ROWS);
	statIncSyncStep = statmgr.GetSample(HBP_INC_SYNC_STEP);

	statMaxFlushReq = statmgr.GetItemU32(TTC_MAX_FLUSH_REQ);
        statCurrFlushReq = statmgr.GetItemU32(TTC_CURR_FLUSH_REQ);

	statOldestDirtyTime = statmgr.GetItemU32(TTC_OLDEST_DIRTY_TIME);
	statAsyncFlushCount = statmgr.GetItemU32(TTC_ASYNC_FLUSH_COUNT);
}

CCacheProcess::~CCacheProcess()
{
	if(m_pstEmptyNodeFilter != NULL)
		delete m_pstEmptyNodeFilter;
}

int CCacheProcess::SetInsertOrder(int o)
{
	if(nodbMode==true && o == INSERT_ORDER_PURGE) {
		log_error("NoDB server don't support TABLE_DEFINE.ServerOrderInsert = purge");
		return -1;
	}

	if(cacheInfo.syncUpdate==0 && o == INSERT_ORDER_PURGE)
	{
		log_error("AsyncUpdate server don't support TABLE_DEFINE.ServerOrderInsert = purge");
		return -1;
	}
	insertOrder = o;
	if(pstDataProcess)
		pstDataProcess->SetInsertOrder(o);
	return 0;
}

int CCacheProcess::EnableNoDBMode(void)
{
	if(insertOrder==INSERT_ORDER_PURGE)
	{
		log_error("NoDB server don't support TABLE_DEFINE.ServerOrderInsert = purge");
		return -1;
	}
	if(tableDef->HasAutoIncrement())
	{
		log_error("NoDB server don't support auto_increment field");
		return -1;
	}
	nodbMode = true;
	fullMode = true;
	return 0;
}

int CCacheProcess::DisableLRUUpdate(int level)
{
	if(level > LRU_WRITE)
		level = LRU_WRITE;
	if(level < 0)
		level = 0;
	noLRU = level;
	return 0;
}

int CCacheProcess::DisableAsyncLog(int disable)
{
	noAsyncLog = !!disable;
	return 0;
}

int CCacheProcess::cache_set_size(unsigned long cacheSize, unsigned int createVersion)
{
	cacheInfo.Init(tableDef->KeyFormat(), cacheSize, createVersion);
	return 0;
}

/*
 * Function		: cache_open
 * Description	: 打开cache
 * Input			: iIpcKey		共享内存ipc key
 *				  ulNodeTotal_	数据节点总数
 ulBucketTotal	hash桶总数
 ulChunkTotal	chunk节点总数
 ulChunkSize	chunk节点大小(单位:byte)
 * Output		: 
 * Return		: 成功返回0,失败返回-1
 */
int CCacheProcess::cache_open( int iIpcKey, int iEnableEmptyFilter, int iEnableAutoDeleteDirtyShm)
{
	cacheInfo.keySize = tableDef->KeyFormat();
	cacheInfo.ipcMemKey = iIpcKey;
	cacheInfo.syncUpdate = !asyncServer;
	cacheInfo.emptyFilter = iEnableEmptyFilter?1:0;
	cacheInfo.autoDeleteDirtyShm = iEnableAutoDeleteDirtyShm?1:0;

	log_debug("cache_info: \n\tshmkey[%d] \n\tshmsize["UINT64FMT"] \n\tkeysize[%u]"
			"\n\tversion[%u] \n\tsyncUpdate[%u] \n\treadonly[%u]"
			"\n\tcreateonly[%u] \n\tempytfilter[%u] \n\tautodeletedirtysharememory[%u]",
			cacheInfo.ipcMemKey, cacheInfo.ipcMemSize, cacheInfo.keySize,
			cacheInfo.version, cacheInfo.syncUpdate, cacheInfo.readOnly, 
			cacheInfo.createOnly, cacheInfo.emptyFilter, cacheInfo.autoDeleteDirtyShm);

	if (Cache.CacheOpen(&cacheInfo)){
		log_error("%s", Cache.Error());
		return -1;
	}

	log_info("Current cache memory format is V%d\n", cacheInfo.version);

	int iMemSyncUpdate = Cache.DirtyLruEmpty()?1:0;
	/*change -- newman
	 * 1. sync ttc + dirty mem, SYNC + mem_dirty
	 * 2. sync ttc + clean mem, SYNC + !mem_dirty
	 * 3. async ttc + dirty mem/clean mem: ASYNC
	 * disable ASYNC <--> FLUSH switch, so FLUSH never happen forever
	 * updateMode == asyncServer
	 * */
	switch(asyncServer*0x10000+iMemSyncUpdate)
	{
		case 0x00000: // sync ttcd + async mem
			mem_dirty = true;
			updateMode = MODE_SYNC;
			break;
		case 0x00001: // sync ttcd + sync mem
			updateMode = MODE_SYNC;
			break;
		case 0x10000: // async ttcd + async mem
			updateMode = MODE_ASYNC;
			break;
		case 0x10001: // async ttcd + sync mem
			updateMode = MODE_ASYNC;
			break;
		default:
			updateMode = cacheInfo.syncUpdate ? MODE_SYNC : MODE_ASYNC;
	}

	if(tableDef->HasAutoIncrement()==0 && updateMode==MODE_ASYNC)
		insertMode = MODE_ASYNC;

	log_info("Cache Update Mode: %s",
			updateMode==MODE_SYNC ? "SYNC" :
			updateMode==MODE_ASYNC ? "ASYNC" :
			updateMode==MODE_FLUSH ? "FLUSH" : "<BAD>");

	// 空结点过滤
	const FEATURE_INFO_T* pstFeature; 
	pstFeature = Cache.QueryFeatureById(EMPTY_FILTER);
	if(pstFeature != NULL){
		NEW(CEmptyNodeFilter, m_pstEmptyNodeFilter);
		if(m_pstEmptyNodeFilter == NULL){
			log_error("new %s error: %m", "CEmptyNodeFilter");
			return -1;
		}
		if(m_pstEmptyNodeFilter->Attach(pstFeature->fi_handle) != 0){
			log_error("EmptyNodeFilter attach error: %s", m_pstEmptyNodeFilter->Error());
			return -1;
		}
	}

	CMallocator* pstMalloc = CBinMalloc::Instance();
	CUpdateMode stUpdateMod = {asyncServer, updateMode, insertMode, insertOrder};
	if(tableDef->IndexFields() > 0){
#if HAS_TREE_DATA
		log_debug("tree index enable, index field num[%d]", tableDef->IndexFields());
		pstDataProcess = new CTreeDataProcess(pstMalloc, tableDef, &Cache, &stUpdateMod);
		if(pstDataProcess == NULL){
			log_error("create CTreeDataProcess error: %m");
			return -1;
		}
#else
		log_error("tree index not supported, index field num[%d]", tableDef->IndexFields());
		return -1;
#endif
	}
	else{
		log_debug("%s", "use raw-data mode");
		pstDataProcess = new CRawDataProcess(pstMalloc, tableDef, &Cache, &stUpdateMod);
		if(pstDataProcess == NULL){
			log_error("create CRawDataProcess error: %m");
			return -1;
		}
		((CRawDataProcess*)pstDataProcess)->SetLimitNodeSize(nodeSizeLimit);
	}

	if(updateMode==MODE_SYNC) {
		noAsyncLog = 1;
	} 

	//Hot Backup
	//设置hblog的路径，并初始化LogWriter, LogReader
	if(hbLog.Init("../log/hblog", "hblog"))
	{
		log_error("cache_open for hblog init failed");
		return -1;
	}

	if(lruLog.Init(hbLog.LogWriter()))
	{
		log_error("cache_open for lrulog init failed");
		return -1;
	}

	// 热备特性
	pstFeature = Cache.QueryFeatureById(HOT_BACKUP);
	if(pstFeature != NULL){
		NEW(CHBFeature, hbFeature);
		if(hbFeature == NULL){
			log_error("new hot-backup feature error: %m");
			return -1;
		}
		if(hbFeature->Attach(pstFeature->fi_handle) != 0){
			log_error("hot-backup feature attach error: %s", hbFeature->Error());
			return -1;
		}

		if(hbFeature->MasterUptime() != 0){
			//开启变更key日志
			hbLog.EnableLog();

			//开启lru调整日志
			lruLog.EnableLog();
		}
	}
	//Hot Backup

	//DelayPurge
	Cache.StartDelayPurgeTask(owner->GetTimerListByMSeconds(10/*10 ms*/));

	//Blacklist
	blacklist_timer = owner->GetTimerList(10*60); /* 10 min sched*/

	NEW(CBlackListUnit(blacklist_timer), blacklist);
	if(NULL == blacklist || blacklist->init_blacklist(100000, tableDef->KeyFormat()))
	{
		log_error("init blacklist failed");
		return -1;
	}

	blacklist->start_blacklist_expired_task();
	//Blacklist

	//Empty Node list
	if(fullMode==true) {
		// nodb Mode has not empty nodes,
		nodeEmptyLimit = 0;
		// prune all present empty nodes
		Cache.PruneEmptyNodeList();
	} else if(nodeEmptyLimit) {
		// Enable Empty Node Limitation
		Cache.SetEmptyNodeLimit(nodeEmptyLimit);
		// re-counting empty node count
		Cache.InitEmptyNodeList();
		// upgrade from old memory
		Cache.UpgradeEmptyNodeList();
		// shrinking empty list
		Cache.ShrinkEmptyNodeList();
	} else {
		// move all empty node to clean list
		Cache.MergeEmptyNodeList();
	}
	//Empty Node list
	return 0;
}


bool CCacheProcess::InsertEmptyNode(void)
{
	for(int i=0; i<2; i++)
	{
		m_stNode = Cache.CacheAllocate(ptrKey);
		if(!(!m_stNode))
			break;

		if(Cache.TryPurgeSize(1, m_stNode) != 0)
			break;
	}
	if(!m_stNode)
	{
		log_debug("alloc cache node error");
		return false;
	}
	m_stNode.VDHandle() = INVALID_HANDLE;
	// new node created, it's EmptyButInCleanList
	nodeEmpty = 0; // means it's not in empty before transation
	return true;
}

TCacheResult CCacheProcess::InsertDefaultRow(CTaskRequest &Task)
{
	int iRet;
	log_debug("%s","insert default start!");

	if(!m_stNode) {
		//发现空节点
		if(InsertEmptyNode()==false)
		{
			log_warning("alloc cache node error");
			Task.SetError(-EIO, CACHE_SVC, "alloc cache node error");
			return CACHE_PROCESS_ERROR;
		}
		if( m_pstEmptyNodeFilter )
			m_pstEmptyNodeFilter->CLR(Task.IntKey());
	} else {
		uint32_t uiTotalRows = ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(m_stNode.VDHandle())))->TotalRows();
		if(uiTotalRows != 0)
			return CACHE_PROCESS_OK;
	}

	CRowValue stRowValue(Task.TableDefinition());
	stRowValue.DefaultValue(); 

	CRawData stDataRows(&g_stSysMalloc, 1);
	iRet = stDataRows.Init(0, Task.TableDefinition()->KeyFormat(), ptrKey);
	if(iRet != 0){
		log_warning("raw data init error: %d, %s", iRet, stDataRows.GetErrMsg());
		Task.SetError(-ENOMEM, CACHE_SVC, "new raw-data error");
		Cache.PurgeNodeEverything(ptrKey, m_stNode);
		return CACHE_PROCESS_ERROR;
	}
	stDataRows.InsertRow(stRowValue, false, false);
	iRet = pstDataProcess->ReplaceData(&m_stNode, &stDataRows);
	if(iRet != 0){
		log_debug("replace data error: %d, %s", iRet, stDataRows.GetErrMsg());
		Task.SetError(-ENOMEM, CACHE_SVC, "replace data error");
		/*标记加入黑名单*/
		Task.PushBlackListSize(stDataRows.DataSize());
		Cache.PurgeNodeEverything(ptrKey, m_stNode);
		return CACHE_PROCESS_ERROR;
	}

	if(m_stNode.VDHandle() == INVALID_HANDLE){
		log_error("BUG: node[%u] vdhandle=0", m_stNode.NodeID());
		Cache.PurgeNode(Task.PackedKey(), m_stNode);
	}

	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_get_data
 * Description	: 处理get请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_get_data(CTaskRequest &Task)
{
	int iRet;

	log_debug("cache_get_data start ");
	TransationFindNode(Task);

	switch(nodeStat) {
		case NODESTAT_MISSING:
			if(fullMode==false) {
				if(Task.FlagNoCache()!=0)
					Task.MarkAsPassThru();
				return CACHE_PROCESS_NEXT;
			}
			--statGetHits; // FullCache Missing treat as miss
			// FullCache Mode: treat as empty & fallthrough
		case NODESTAT_EMPTY:
			++statGetHits;
			//发现空节点，直接构建result
			log_debug("found Empty-Node[%u], response directed", Task.IntKey());
			Task.PrepareResult();
			Task.SetTotalRows(0);
			Task.SetResultHitFlag(HIT_SUCCESS);
			return CACHE_PROCESS_OK;
	}

	++statGetHits;
	log_debug("[%s:%d]cache hit ",__FILE__,__LINE__);

	TransationUpdateLRU(false, LRU_READ);
	iRet = pstDataProcess->GetData(Task, &m_stNode);
	if(iRet != 0){
		log_error("GetData() failed");
		Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
		return CACHE_PROCESS_ERROR;
	}

	// Hot Backup
	if(noLRU<LRU_READ && lruLog.Set(m_stNode.NodeID()))
	{
		//为避免错误扩大， 给客户端成功响应
		log_crit("hb: log lru key failed");
	}
	// Hot Bakcup	
	Task.SetResultHitFlag(HIT_SUCCESS);
	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_batch_get_data
 * Description	: 处理get请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_batch_get_data(CTaskRequest &Task)
{
	int index;
	int iRet;

	log_debug("cache_batch_get_data start ");

	Task.PrepareResultNoLimit();
	for(index=0; Task.SetBatchCursor(index) >= 0; index++) {
		++statGetCount;
		Task.SetResultHitFlag(HIT_INIT);
		TransationFindNode(Task);
		switch(nodeStat) {
			case NODESTAT_EMPTY:
				++statGetHits;
				Task.DoneBatchCursor(index);
				log_debug("[%s:%d]cache empty ",__FILE__,__LINE__);
				break;
			case NODESTAT_MISSING:
				if(fullMode)
					Task.DoneBatchCursor(index);
				log_debug("[%s:%d]cache miss ",__FILE__,__LINE__);
				break;
			case NODESTAT_PRESENT:
				++statGetHits;
				log_debug("[%s:%d]cache hit ",__FILE__,__LINE__);

				TransationUpdateLRU(false, LRU_BATCH);
				iRet = pstDataProcess->GetData(Task, &m_stNode);
				if(iRet != 0){
					log_error("GetData() failed");
					Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
					return CACHE_PROCESS_ERROR;
				}
				Task.DoneBatchCursor(index);

				// Hot Backup
				if(noLRU<LRU_BATCH && lruLog.Set(m_stNode.NodeID()))
				{
					//为避免错误扩大， 给客户端成功响应
					log_crit("hb: log lru key failed");
				}
				break;
		}
		TransationEnd();
	}
	// Hot Bakcup	
	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_get_rb
 * Description	: 处理Helper的get回读task
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_get_rb(CTaskRequest &Task)
{
	log_debug ("cache_get_rb start ");

	Task.PrepareResult();
	int iRet = Task.AppendResult(Task.result);
	if(iRet < 0) {
		log_notice("task append_result error: %d", iRet);
		Task.SetError(iRet, CACHE_SVC, "AppendResult() error");
		return CACHE_PROCESS_ERROR;
	}
	log_debug ("cache_get_rb success");

	return CACHE_PROCESS_OK;
}

/* helper执行GET回来后，更新内存数据 */
TCacheResult CCacheProcess::cache_replace_result(CTaskRequest &Task)
{
	int iRet;
	int oldRows = 0;

	log_debug("cache replace all start!");

	TransationFindNode(Task);

	//数据库回来的记录如果是0行则
	// 1. 设置bits 2. 直接构造0行的result响应包
	if(m_pstEmptyNodeFilter != NULL) {
		if((Task.result == NULL || Task.result->TotalRows() == 0)){
			log_debug("SET Empty-Node[%u]", Task.IntKey());
			m_pstEmptyNodeFilter->SET(Task.IntKey());
			Cache.CachePurge(ptrKey);
			return CACHE_PROCESS_OK;
		} else {
			m_pstEmptyNodeFilter->CLR(Task.IntKey());
		}
	}

	if(!m_stNode){
		if(InsertEmptyNode()==false)
			return CACHE_PROCESS_OK;
	} else {
		oldRows = Cache.NodeRowsCount(m_stNode);
	}

	unsigned int uiNodeID = m_stNode.NodeID();
	iRet = 	pstDataProcess->ReplaceData(Task, &m_stNode);
	if(iRet != 0 || m_stNode.VDHandle() == INVALID_HANDLE){
		if(nodbMode==true) {
			/* UNREACHABLE */
			log_info("cache replace data error: %d. node: %u", iRet, uiNodeID);
			Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
			return CACHE_PROCESS_ERROR;
		}
		log_debug("cache replace data error: %d. purge node: %u", iRet, uiNodeID);
		Cache.PurgeNodeEverything(ptrKey, m_stNode);
		Cache.IncDirtyRow(0-oldRows);
		return CACHE_PROCESS_OK;
	}
	Cache.IncTotalRow(pstDataProcess->RowsInc());

	TransationUpdateLRU(false, LRU_READ);
	if(oldRows != 0 || Cache.NodeRowsCount(m_stNode) != 0) {
		// Hot Backup
		if(noLRU<LRU_READ && lruLog.Set(m_stNode.NodeID())){
			//为避免错误扩大， 给客户端成功响应
			log_crit("hb: log lru key failed");
		}
		// Hot Bakcup
	}

	log_debug("cache_replace_result success! ");

	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_flush_data
 * Description	: 处理flush请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_flush_data_before_delete(CTaskRequest &Task)
{
		log_debug("%s","flush start!");
		TransationFindNode(Task);
		if(!m_stNode || !(m_stNode.IsDirty()))
		{
				log_debug("node is null or node is clean,return CACHE_PROCESS_OK");
				return CACHE_PROCESS_OK;
		}
		unsigned int uiFlushRowsCnt;

		CNode node = m_stNode;
		int iRet = 0;

		/*init*/
		keyDirty=m_stNode.IsDirty();	

		CFlushRequest * flushReq = new CFlushRequest(this, ptrKey);
		if(flushReq == NULL)
		{
				log_error("new CFlushRequest error: %m");
				return CACHE_PROCESS_ERROR;
		}

		iRet = pstDataProcess->FlushData(flushReq, &m_stNode, uiFlushRowsCnt);
		if (iRet != 0)
		{
				log_error("FlushData error:%d",iRet);
				return CACHE_PROCESS_ERROR;
		}
		if(uiFlushRowsCnt == 0)
		{
				delete flushReq;
				if(keyDirty)
						Cache.IncDirtyNode(-1);
				m_stNode.ClrDirty();
				Cache.RemoveFromLru(m_stNode);
				Cache.Insert2CleanLru(m_stNode);
				return CACHE_PROCESS_OK;
		}else
		{
				CommitFlushRequest(flushReq, NULL);
				Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());
				if(keyDirty)
						Cache.IncDirtyNode(-1);
				m_stNode.ClrDirty();
				Cache.RemoveFromLru(m_stNode);
				Cache.Insert2CleanLru(m_stNode);
				++statFlushCount;
				statFlushRows += uiFlushRowsCnt;
				return CACHE_PROCESS_OK;
		}

}

/*
 * Function		: cache_flush_data
 * Description	: 处理flush请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_flush_data(CTaskRequest &Task)
{

	log_debug("%s","flush start!");
	TransationFindNode(Task);
	if(!m_stNode || !(m_stNode.IsDirty()))
		return CACHE_PROCESS_OK;

	unsigned int uiFlushRowsCnt;

	TCacheResult iRet = cache_flush_data(m_stNode, &Task, uiFlushRowsCnt);
	if(iRet == CACHE_PROCESS_OK){
		++statFlushCount;
		statFlushRows += uiFlushRowsCnt;
	}
	return(iRet);
}

/*called by flush next node*/
int CCacheProcess::cache_flush_data_timer(CNode& stNode, unsigned int& uiFlushRowsCnt)
{
	int iRet, err = 0;

	/*init*/
	TransationBegin(NULL);
	keyDirty=stNode.IsDirty();	
	ptrKey = ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(stNode.VDHandle())))->Key();

	CFlushRequest * flushReq = new CFlushRequest(this, ptrKey);
	if(flushReq == NULL)
	{
	    log_error("new CFlushRequest error: %m");
	    err = -1;
	    goto __out;
	}

	iRet = pstDataProcess->FlushData(flushReq, &stNode, uiFlushRowsCnt);

	if(uiFlushRowsCnt == 0)
	{
	    delete flushReq;
	    if(iRet < 0)
	    {
		err = -2;
		goto __out;
	    }else
	    {
		if(keyDirty)
		    Cache.IncDirtyNode(-1);
		stNode.ClrDirty();
		Cache.RemoveFromLru(stNode);
		Cache.Insert2CleanLru(stNode);
		err = 1;
		goto __out;
	    }
	}else
	{
	    CommitFlushRequest(flushReq, NULL);
	    Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());
	    if(iRet == 0)
	    {
		if(keyDirty)
		    Cache.IncDirtyNode(-1);
		stNode.ClrDirty();
		Cache.RemoveFromLru(stNode);
		Cache.Insert2CleanLru(stNode);
		err = 2;
		goto __out;
	    }else
	    {
		err = -5;
		goto __out;
	    }
	}

__out:
	/*clear init*/
	CCacheTransation::Free();
	return err;
}
/*
 * Function		: cache_flush_data
 * Description	: 处理flush请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_flush_data(CNode& stNode, CTaskRequest* pstTask, unsigned int& uiFlushRowsCnt)
{
	int iRet;

	/*could called by flush timer event, no transationFindNode called there, can't trust KeyDirty, recal it-- by newman*/
	keyDirty=stNode.IsDirty();	

	log_debug("%s","flush node start!");


	int flushCnt = 0;
	CFlushRequest *flushReq = NULL;
	if(!nodbMode) {
		flushReq = new CFlushRequest(this, ptrKey);
		if(flushReq == NULL){
			log_error("new CFlushRequest error: %m");
			if(pstTask != NULL)
				pstTask->SetError(-ENOMEM, CACHE_SVC, "new CFlushRequest error");
			return CACHE_PROCESS_ERROR;
		}
	}

	iRet = pstDataProcess->FlushData(flushReq, &stNode, uiFlushRowsCnt);

	if(flushReq) {
		flushCnt = flushReq->numReq;
		CommitFlushRequest(flushReq, pstTask);
		if(iRet != 0){
			log_error("FlushData() failed while flush data");
			if(pstTask != NULL)
				pstTask->SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());

			return CACHE_PROCESS_ERROR;
		}
	}

	Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());

	if(keyDirty)
		Cache.IncDirtyNode(-1);

	stNode.ClrDirty();
	keyDirty = 0;
	TransationUpdateLRU(false, LRU_ALWAYS);

	log_debug("cache_flush_data success");
	if(flushCnt == 0)
		return CACHE_PROCESS_OK;
	else
		return CACHE_PROCESS_PENDING;
}


/*
 * Function		: cache_purge_data
 * Description	: 处理purge请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_purge_data(CTaskRequest &Task)
{
	TransationFindNode(Task);

	switch(nodeStat) {
		case NODESTAT_EMPTY:
			m_pstEmptyNodeFilter->CLR(Task.IntKey());
			return CACHE_PROCESS_OK;
			
		case NODESTAT_MISSING:
			return CACHE_PROCESS_OK;

		case NODESTAT_PRESENT:
			break;
	}

	TCacheResult iRet = CACHE_PROCESS_OK;
	if(updateMode && m_stNode.IsDirty()){
		unsigned int uiFlushRowsCnt;
		iRet = cache_flush_data(m_stNode, &Task, uiFlushRowsCnt);
		if(iRet != CACHE_PROCESS_PENDING)
			return iRet;
	}

	++statDropCount;
	statDropRows += ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(m_stNode.VDHandle())))->TotalRows();
	Cache.IncTotalRow( 0LL - ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(m_stNode.VDHandle())))->TotalRows() );

	unsigned int uiNodeID = m_stNode.NodeID();
	if(Cache.CachePurge(ptrKey) != 0){
		log_error("PANIC: purge node[id=%u] fail", uiNodeID);
	}

	return iRet;
}

/*
 * Function		: cache_update_rows
 * Description	: 处理Helper的update task
 * Input		: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_update_rows(CTaskRequest &Task, bool async, bool setrows)
{
	int iRet;

	log_debug("cache update data start! ");	

	if(m_bReplaceEmpty == true){
		TCacheResult ret = InsertDefaultRow(Task);
		if(ret != CACHE_PROCESS_OK)
			return(ret);
	}


	int rows = Cache.NodeRowsCount(m_stNode);
	iRet = pstDataProcess->UpdateData(Task, &m_stNode, pstLogRows, async, setrows);
	if(iRet != 0){
		if(async==false && !Task.FlagBlackHole())
		{
			Cache.PurgeNodeEverything(ptrKey, m_stNode);
			Cache.IncTotalRow(0LL-rows);
			return CACHE_PROCESS_OK;
		}
		log_warning("UpdateData() failed: %d,%s", iRet, pstDataProcess->GetErrMsg());
		Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
		TransationUpdateLRU(async, LRU_ALWAYS);
		goto ERR_RETURN;
	}
    /*if update volatile field,node won't be dirty
     * add by foryzhou*/
	TransationUpdateLRU((Task.resultInfo.AffectedRows()>0 &&
                (Task.RequestOperation() && Task.RequestOperation()->HasTypeCommit())//has core field modified
                ) ? async: false, LRU_WRITE);

	Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());

	// Hot Backup
	if(nodeStat != NODESTAT_PRESENT ||
			(Task.RequestOperation() && Task.RequestOperation()->HasTypeCommit()))
	{
	// only write log if some non-volatile field got updated
	// or cache miss and m_bReplaceEmpty is set (equiv insert(default)+update)
	if(WriteHBLog(Task, m_stNode, CHotBackup::SYNC_UPDATE))
	{
		//为避免错误扩大， 给客户端成功响应
		log_crit("hb: log update key failed");
	}
	}
	// Hot Bakcup

	return CACHE_PROCESS_OK;

ERR_RETURN:
	return CACHE_PROCESS_ERROR;
}

/* cache_replace_rows don't allow empty stNode */
TCacheResult CCacheProcess::cache_replace_rows(CTaskRequest &Task, bool async, bool setrows)
{
	int iRet;

	log_debug("cache replace rows start!");

	int rows = Cache.NodeRowsCount(m_stNode);
	iRet = 	pstDataProcess->ReplaceRows(Task, &m_stNode, pstLogRows, async, setrows);
	if(iRet != 0){
		if(keyDirty == false && !Task.FlagBlackHole()) {
			Cache.PurgeNodeEverything(ptrKey, m_stNode);
			Cache.IncTotalRow(0LL-rows);
		}
		
		/* 如果是同步replace命令，返回成功*/
		if(async==false && !Task.FlagBlackHole())
			return CACHE_PROCESS_OK;

		log_error("cache replace rows error: %d,%s", iRet, pstDataProcess->GetErrMsg());
		Task.SetError(-EIO, CACHE_SVC, "ReplaceData() error");
		return CACHE_PROCESS_ERROR;	
	}
	Cache.IncTotalRow(pstDataProcess->RowsInc());
	Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());

	TCacheResult ret = CACHE_PROCESS_OK;

	TransationUpdateLRU(async, LRU_WRITE);

	// Hot Backup
	if(WriteHBLog(Task, m_stNode, CHotBackup::SYNC_UPDATE))
		//    if(hbLog.WriteUpdateKey(Task.PackedKey(), CHotBackup::SYNC_UPDATE))
	{
		//为避免错误扩大， 给客户端成功响应
		log_crit("hb: log update key failed");
	}
	// Hot Bakcup

	log_debug("cache_replace_rows success! ");

	if(m_stNode.VDHandle() == INVALID_HANDLE){
		log_error("BUG: node[%u] vdhandle=0", m_stNode.NodeID());
		Cache.PurgeNode(Task.PackedKey(), m_stNode);
		Cache.IncTotalRow(0LL-rows);
	}

	return ret;
}


/*
 * Function	: cache_insert_row
 * Description	: 处理Helper的insert task
 * Input		: Task			请求信息
 * Output	: Task			返回信息
 * Return	: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_insert_row(CTaskRequest &Task, bool async, bool setrows)
{
	int iRet;

	if(!m_stNode) {
		if(InsertEmptyNode() == false) {
			if(async==true || Task.FlagBlackHole()) {
				Task.SetError(-EIO, CACHE_SVC, "AllocateNode Error while insert row");
				return CACHE_PROCESS_ERROR;
			}
			return CACHE_PROCESS_OK;
		}

		CRawData stDataRows(&g_stSysMalloc, 1);
		iRet = stDataRows.Init(0, Task.TableDefinition()->KeyFormat(), ptrKey);
		if(iRet != 0){
			log_warning("raw data init error: %d, %s", iRet, stDataRows.GetErrMsg());
			Task.SetError(-ENOMEM, CACHE_SVC, "new raw-data error");
			Cache.PurgeNodeEverything(ptrKey, m_stNode);
			return CACHE_PROCESS_ERROR;
		}
		iRet = pstDataProcess->ReplaceData(&m_stNode, &stDataRows);
		if(iRet != 0){
			log_warning("raw data init error: %d, %s", iRet, stDataRows.GetErrMsg());
			Task.SetError(-ENOMEM, CACHE_SVC, "new raw-data error");
			Cache.PurgeNodeEverything(ptrKey, m_stNode);
			return CACHE_PROCESS_ERROR;
		}

		if( m_pstEmptyNodeFilter )
			m_pstEmptyNodeFilter->CLR(Task.IntKey());
	}

	int oldRows = Cache.NodeRowsCount(m_stNode);
	iRet = pstDataProcess->AppendData(Task, &m_stNode, pstLogRows, async, setrows);
	if(iRet == -1062) {
		Task.SetError(-ER_DUP_ENTRY,CACHE_SVC,"duplicate unique key detected");
		return CACHE_PROCESS_ERROR;
	} else if(iRet != 0){
		if(async==false && !Task.FlagBlackHole()) {
			log_debug("AppendData() failed, purge now [%d %s]",iRet, pstDataProcess->GetErrMsg());
			Cache.IncTotalRow(0LL-oldRows);
			Cache.PurgeNodeEverything(ptrKey, m_stNode);
			return CACHE_PROCESS_OK;
		} else {
			log_error("AppendData() failed while update data");
			Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
			return CACHE_PROCESS_ERROR;
		}
	}
	TransationUpdateLRU(async, LRU_WRITE);

	Cache.IncTotalRow(pstDataProcess->RowsInc());
	if(async==true)
		Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());

	// Hot Backup
	if(WriteHBLog(Task, m_stNode, CHotBackup::SYNC_INSERT))
		//    if(hbLog.WriteUpdateKey(Task.PackedKey(), CHotBackup::SYNC_INSERT))
	{
		//为避免错误扩大， 给客户端成功响应
		log_crit("hb: log update key failed");
	}
	// Hot Bakcup

	log_debug("cache_insert_row success");
	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_delete_rows
 * Description	: 处理del请求
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 成功返回0,失败返回-1
 */
TCacheResult CCacheProcess::cache_delete_rows(CTaskRequest &Task)
{
    int iRet;

    log_debug("cache_delete_rows start! ");

    uint32_t oldRows = Cache.NodeRowsCount(m_stNode);

    int all_row_delete = Task.AllRows();

    if(Task.AllRows() != 0){ //如果没有del条件则删除整个节点
empty:
	if (lossyMode || Task.FlagBlackHole())
	{
	    Task.resultInfo.SetAffectedRows(oldRows);
	}

	/*row cnt statistic dec by 1*/
	Cache.IncTotalRow( 0LL - oldRows );
	
	/*dirty node cnt staticstic dec by 1 -- by newman*/
	if(keyDirty)
	{
	    Cache.IncDirtyNode(-1);
	}

	/* dirty row cnt statistic dec, if count dirty row error, let statistic wrong with it -- newman*/
	if(all_row_delete)
	{
	    int old_dirty_rows = pstDataProcess->DirtyRowsInNode(Task, &m_stNode);
	    if(old_dirty_rows > 0)
	        Cache.IncDirtyRow(old_dirty_rows);
	}else{
	    Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());    
	}

	//Cache.CachePurge(ptrKey); node cnt statistic dec by 1
	Cache.PurgeNodeEverything(ptrKey, m_stNode);
	if( m_pstEmptyNodeFilter )
	    m_pstEmptyNodeFilter->SET(Task.IntKey());

	// Hot Backup
	CNode stEmpytNode;
	if(WriteHBLog(Task, stEmpytNode, CHotBackup::SYNC_PURGE))
	    //		if(hbLog.WriteUpdateKey(Task.PackedKey(), CHotBackup::SYNC_UPDATE))
	{
	    //为避免错误扩大， 给客户端成功响应
	    log_crit("hb: log update key failed");
	}
	// Hot Bakcup

	return CACHE_PROCESS_OK;
    }

    /*delete error handle is too simple, statistic can not trust if error happen here -- by newman*/
    iRet = pstDataProcess->DeleteData(Task, &m_stNode, pstLogRows);
    if(iRet != 0){
	log_error("DeleteData() failed: %d,%s", iRet, pstDataProcess->GetErrMsg());
	Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
	if(!keyDirty){
	    Cache.IncTotalRow( 0LL - oldRows );
	    Cache.PurgeNodeEverything(ptrKey, m_stNode);
	}
	return CACHE_PROCESS_ERROR;
    }

    /* Delete to empty */
    uint32_t uiTotalRows = ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(m_stNode.VDHandle())))->TotalRows();
    if(uiTotalRows == 0)
	goto empty;

    Cache.IncDirtyRow(pstDataProcess->DirtyRowsInc());
    Cache.IncTotalRow(pstDataProcess->RowsInc());

    TransationUpdateLRU(false, LRU_WRITE);

    // Hot Backup
    if(WriteHBLog(Task, m_stNode, CHotBackup::SYNC_DELETE))
	//    if(hbLog.WriteUpdateKey(Task.PackedKey(), CHotBackup::SYNC_UPDATE))
    {
	//为避免错误扩大， 给客户端成功响应
	log_crit("hb: log update key failed");
    }
    // Hot Bakcup

    return CACHE_PROCESS_OK;
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::check_allowed_insert(CTaskRequest &Task)
{
    int rows = Cache.NodeRowsCount(m_stNode);
    // single rows checker
    if(tableDef->KeyAsUniqField() && rows != 0) {
	Task.SetError(-ER_DUP_ENTRY,CACHE_SVC,"duplicate unique key detected");
	return CACHE_PROCESS_ERROR;
    }
    if(nodeRowsLimit > 0 && rows >= nodeRowsLimit) {
	/* check weather allowed execute insert operation*/
	Task.SetError(-EC_NOT_ALLOWED_INSERT, __FUNCTION__,
		"rows exceed limit, not allowed insert any more data" );
	return CACHE_PROCESS_ERROR;
    }
    return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_sync_insert_precheck (CTaskRequest& Task)
{
	log_debug ("%s", "cache_sync_insert begin");
	if(m_bReplaceEmpty == true){ // 这种模式下，不支持insert操作
		Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"insert cmd from client, not support under replace mode");
		log_notice("insert cmd from client, not support under replace mode");
		return CACHE_PROCESS_ERROR;
	}

	if(tableDef->KeyAsUniqField() || nodeRowsLimit > 0) { 
		TransationFindNode(Task);

		// single rows checker
		if(nodeStat==NODESTAT_PRESENT && check_allowed_insert(Task)==CACHE_PROCESS_ERROR)
			return CACHE_PROCESS_ERROR;
	}

	return CACHE_PROCESS_NEXT; 
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_sync_insert (CTaskRequest& Task)
{
	log_debug ("%s", "cache_sync_insert begin");
	if(m_bReplaceEmpty == true){ // 这种模式下，不支持insert操作
		Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"insert cmd from client, not support under replace mode");
		log_notice("insert cmd from client, not support under replace mode");
		return CACHE_PROCESS_ERROR;
	}

	if(Task.resultInfo.InsertID() > 0)
		Task.UpdatePackedKey(Task.resultInfo.InsertID()); // 如果自增量字段是key，则会更新key

	TransationFindNode(Task);

	// Missing is NO-OP, otherwise insert it
	switch(nodeStat) {
		case NODESTAT_MISSING:
			return CACHE_PROCESS_OK; 

		case NODESTAT_EMPTY:
		case NODESTAT_PRESENT:
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			break;
	}

	return cache_insert_row(Task, false /* async */, lossyMode /* setrows */);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_sync_update (CTaskRequest& Task)
{
	bool setrows = lossyMode;
	log_debug ("%s", "cache_sync_update begin");
	// NOOP sync update
	if(Task.RequestOperation()==NULL) {
		// no field need to update
		return CACHE_PROCESS_OK; //如果helper更新的纪录数为0则直接返回
	} else if (setrows==false && Task.resultInfo.AffectedRows() == 0) {
		if(Task.RequestOperation()->HasTypeCommit()==0) {
			// pure volatile update, ignore upstream affected-rows
			setrows = true;
		} else if(Task.RequestCondition() && Task.RequestCondition()->HasTypeTimestamp()) {
			// update base timestamp fields, ignore upstream affected-rows
			setrows = true;
		} else {
			log_debug("%s","helper's affected rows is zero");
			return CACHE_PROCESS_OK; //如果helper更新的纪录数为0则直接返回
		}
	}


	TransationFindNode(Task);

	// Missing or Empty is NO-OP except EmptyAsDefault logical
	switch(nodeStat) {
		case NODESTAT_MISSING:
			return CACHE_PROCESS_OK; 

		case NODESTAT_EMPTY:
			if(m_bReplaceEmpty == true)
				break;
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			return CACHE_PROCESS_OK; 

		case NODESTAT_PRESENT:
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			break;
	}

	return cache_update_rows (Task, false /*Async*/, setrows);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_sync_replace (CTaskRequest& Task)
{
	const int setrows = lossyMode;
	log_debug ("%s", "cache_sync_replace begin");
	// NOOP sync update
	if (lossyMode==false && Task.resultInfo.AffectedRows() == 0) 
	{
		log_debug("%s","helper's affected rows is zero");
		return CACHE_PROCESS_OK; //如果helper更新的纪录数为0则直接返回
	}

	TransationFindNode(Task);

	// missing node is NO-OP, empty node insert it, otherwise replace it
	switch(nodeStat) {
		case NODESTAT_MISSING:
			return CACHE_PROCESS_OK; 

		case NODESTAT_EMPTY:
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			return cache_insert_row(Task, false, setrows);

		case NODESTAT_PRESENT:
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			break;
	}

	return cache_replace_rows(Task, false, lossyMode);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_sync_delete (CTaskRequest& Task)
{
	log_debug ("%s", "cache_sync_delete begin");
	// didn't check zero AffectedRows 
	TransationFindNode(Task);

	// missing and empty is NO-OP, otherwise delete it
	switch(nodeStat) {
		case NODESTAT_MISSING:
            return CACHE_PROCESS_OK;
		case NODESTAT_EMPTY:
            if(lossyMode) {
                Task.SetError(0, NULL, NULL);
                Task.resultInfo.SetAffectedRows(0);
            }
			return CACHE_PROCESS_OK; 

		case NODESTAT_PRESENT:
			break;
	}

	return cache_delete_rows(Task);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_nodb_insert (CTaskRequest& Task)
{
	log_debug ("%s", "cache_asyn_prepare_insert begin");
	if(m_bReplaceEmpty == true){ // 这种模式下，不支持insert操作
		Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"insert cmd from client, not support under replace mode");
		log_notice("insert cmd from client, not support under replace mode");
		return CACHE_PROCESS_ERROR;
	}

	TransationFindNode(Task);
	if(nodeStat == NODESTAT_PRESENT && check_allowed_insert(Task)==CACHE_PROCESS_ERROR)
		return CACHE_PROCESS_ERROR;

	return cache_insert_row(Task, false /* async */, true /* setrows */);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_nodb_update (CTaskRequest& Task)
{
	log_debug ("%s", "cache_fullmode_prepare_update begin");

	TransationFindNode(Task);

	// missing & empty is NO-OP, except EmptyAsDefault logical
	switch(nodeStat) {
		case NODESTAT_MISSING:
		case NODESTAT_EMPTY:
			if(m_bReplaceEmpty == true)
				break;
			return CACHE_PROCESS_OK; 

		case NODESTAT_PRESENT:
			break;
	}

	return cache_update_rows (Task, false /*Async*/, true /*setrows*/);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_nodb_replace (CTaskRequest& Task)
{
	log_debug ("%s", "cache_asyn_prepare_replace begin");
	TransationFindNode(Task);

	// missing & empty insert it, otherwise replace it
	switch(nodeStat) {
		case NODESTAT_EMPTY:
		case NODESTAT_MISSING:
			return cache_insert_row(Task, false, true /* setrows */);

		case NODESTAT_PRESENT:
			break;
	}

	return cache_replace_rows(Task, false, true);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_nodb_delete (CTaskRequest& Task)
{
	log_debug ("%s", "cache_fullmode_delete begin");
	TransationFindNode(Task);

	// missing & empty is NO-OP
	switch(nodeStat) {
		case NODESTAT_MISSING:
		case NODESTAT_EMPTY:
			return CACHE_PROCESS_OK; 

		case NODESTAT_PRESENT:
			break;
	}

	return cache_delete_rows(Task);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

TCacheResult CCacheProcess::cache_async_insert (CTaskRequest& Task)
{
	log_debug ("%s", "cache_async_insert begin");
	if(m_bReplaceEmpty == true){ // 这种模式下，不支持insert操作
		Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"insert cmd from client, not support under replace mode");
		log_notice("insert cmd from client, not support under replace mode");
		return CACHE_PROCESS_ERROR;
	}

	TransationFindNode(Task);
	switch(nodeStat) {
		case NODESTAT_MISSING:
			if(fullMode==false)
				return CACHE_PROCESS_NEXT; 
			if(updateMode==MODE_FLUSH)
				return CACHE_PROCESS_NEXT; 
			break;

		case NODESTAT_EMPTY:
			if(updateMode==MODE_FLUSH)
				return CACHE_PROCESS_NEXT; 
			break;

		case NODESTAT_PRESENT:
			if(check_allowed_insert(Task)==CACHE_PROCESS_ERROR)
				return CACHE_PROCESS_ERROR;
			if(updateMode==MODE_FLUSH && !(m_stNode.IsDirty()))
				return CACHE_PROCESS_NEXT; 
			break;
	}

	log_debug ("%s", "cache_async_insert data begin");
	//对insert 操作命中数据进行采样
	++ statInsertHits;

	return cache_insert_row(Task, true /* async */, true /* setrows */);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_async_update (CTaskRequest& Task)
{
	log_debug ("%s", "cache_asyn_update begin");

	TransationFindNode(Task);
	switch(nodeStat) {
		case NODESTAT_MISSING:
			if(fullMode==false)
				return CACHE_PROCESS_NEXT; 
			// FALLTHROUGH
		case NODESTAT_EMPTY:
			if(m_bReplaceEmpty == true){
				if(updateMode==MODE_FLUSH)
					return CACHE_PROCESS_NEXT; 
				break;
			}
			return CACHE_PROCESS_OK; 

		case NODESTAT_PRESENT:
			if(updateMode==MODE_FLUSH && !(m_stNode.IsDirty()))
				return CACHE_PROCESS_NEXT; 
			break;
	}

	log_debug ("%s", "cache_async_update update data begin");
	//对update 操作命中数据进行采样
	++ statUpdateHits;

	return cache_update_rows (Task, true /*Async*/, true /*setrows*/);
}

//----------------------------------------------------------------------------
TCacheResult CCacheProcess::cache_async_replace (CTaskRequest& Task)
{
	log_debug ("%s", "cache_asyn_prepare_replace begin");
	TransationFindNode(Task);

	switch(nodeStat) {
		case NODESTAT_MISSING:
			if(fullMode==false)
				return CACHE_PROCESS_NEXT; 
			if(updateMode==MODE_FLUSH)
				return CACHE_PROCESS_NEXT; 
			if(tableDef->KeyAsUniqField()==false)
				return CACHE_PROCESS_NEXT;
			return cache_insert_row(Task, true, true);

		case NODESTAT_EMPTY:
			if(updateMode==MODE_FLUSH)
				return CACHE_PROCESS_NEXT; 
			return cache_insert_row(Task, true, true);
			
		case NODESTAT_PRESENT:
			if(updateMode==MODE_FLUSH && !(m_stNode.IsDirty()))
				return CACHE_PROCESS_NEXT; 
			break;
	}

	return cache_replace_rows(Task, true, true);
}

/*
 * Function		: cache_process_request
 * Description	: 处理incoming task
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 0 			成功
 *				: -1			失败
 */
TCacheResult CCacheProcess::cache_process_request(CTaskRequest &Task)
{
	Task.RenewTimestamp();
	szErrMsg[0] = 0;

	/* 取命令字 */
	int iCmd = Task.RequestCode();
	 log_debug("CCacheProcess::cache_process_request cmd is %d ",iCmd );
	switch(iCmd){
		case DRequest::Get:
			Task.SetResultHitFlag(HIT_INIT); //set hit flag init status
			if(Task.CountOnly() && (Task.requestInfo.LimitStart() || Task.requestInfo.LimitCount()))
			{
				Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"There's nothing to limit because no fields required");
				return CACHE_PROCESS_ERROR;
			}

			/* 如果命中黑名单，则purge掉当前节点，走PassThru模式 */
			if(blacklist->in_blacklist(Task.PackedKey()))
			{
				/* 
				 * 理论上是在黑名单的节点是不可能在cache中的
				 * 为了防止异常，预purge。
				 */
				log_debug("blacklist hit, passthough to datasource");
				cache_purge_data(Task);
				Task.MarkAsPassThru();
				return CACHE_PROCESS_NEXT;
			}

			log_debug("blacklist miss, normal process");
			
			++statGetCount;
			
			return cache_get_data(Task); 

		case DRequest::Insert:
			++ statInsertCount;

			if(updateMode==MODE_ASYNC && insertMode!=MODE_SYNC)
				return cache_async_insert(Task);

			//标示task将提交给helper
			return cache_sync_insert_precheck(Task);

		case DRequest::Update:
			++ statUpdateCount;

			if(updateMode)
				return cache_async_update(Task);

			//标示task将提交给helper
			return CACHE_PROCESS_NEXT;	

			//如果clinet 上送Delete 操作，删除Cache中数据，同时提交Helper
			//现阶段异步Cache暂时不支持Delete操作
        case DRequest::Delete:
            if(updateMode!=MODE_SYNC){
                if(Task.RequestCondition() && Task.RequestCondition()->HasTypeRW()){
                    Task.SetError(-EC_BAD_ASYNC_CMD,CACHE_SVC,"Delete base non ReadOnly fields");
                    return CACHE_PROCESS_ERROR;
                }
                //异步delete前先flush
                TCacheResult iRet = CACHE_PROCESS_OK;
                iRet = cache_flush_data_before_delete(Task);
                if(iRet == CACHE_PROCESS_ERROR)
                    return iRet;
            }


            //对于delete操作，直接提交DB，不改变原有逻辑
            ++ statDeleteCount;

            //标示task将提交给helper
            return CACHE_PROCESS_NEXT;

		case DRequest::Purge:
			//删除指定key在cache中的数据
			++ statPurgeCount;
			return cache_purge_data(Task);

		case DRequest::Flush:
			if(updateMode)
				//flush指定key在cache中的数据
				return cache_flush_data(Task);
			else
				return CACHE_PROCESS_OK;

		case DRequest::Replace: //如果是淘汰的数据，不作处理
			++ statUpdateCount;

			// 限制key字段作为唯一字段才能使用replace命令
			if( !(Task.TableDefinition()->KeyPartOfUniqField()) || Task.TableDefinition()->HasAutoIncrement()){
				Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"replace cmd require key fields part of uniq-fields and no auto-increment field");
				return CACHE_PROCESS_ERROR;
			}

			if(updateMode)
				return cache_async_replace(Task);

			//标示task将提交给helper
			return CACHE_PROCESS_NEXT;	

		case DRequest::SvrAdmin:
			return cache_process_admin(Task);

		default:
			Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"invalid cmd from client");
			log_notice("invalid cmd[%d] from client", iCmd);
			break;
	}//end of switch

	return CACHE_PROCESS_ERROR;
}

/*
 * Function		: cache_process_batch
 * Description	: 处理incoming batch task
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 0 			成功
 *				: -1			失败
 */
TCacheResult CCacheProcess::cache_process_batch(CTaskRequest &Task)
{
	Task.RenewTimestamp();
	szErrMsg[0] = 0;

	/* 取命令字 */
	int iCmd = Task.RequestCode();
	if(nodeEmptyLimit) {
		int bsize = Task.GetBatchSize();
		if(bsize * 10 > nodeEmptyLimit) {
			Task.SetError(-EC_TOO_MANY_KEY_VALUE, __FUNCTION__, "batch count exceed LimitEmptyNodes/10");
			return CACHE_PROCESS_ERROR;
		}
	}
	switch(iCmd){
		case DRequest::Get:
			return cache_batch_get_data(Task); 

		default: // unknwon command treat as OK, fallback to split mode
			break;
	}//end of switch

	return CACHE_PROCESS_OK;
}

/*
 * Function		: cache_process_reply
 * Description	: 处理task from helper reply
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 0 			成功
 *				: -1			失败
 */

TCacheResult CCacheProcess::cache_process_reply(CTaskRequest &Task)
{
	Task.RenewTimestamp();
	szErrMsg[0] = '\0';
	int iLimit = 0;

	int iCmd = Task.RequestCode();
	switch(iCmd){
		case DRequest::Get: //一定是cache miss,全部replace入cache
			if(Task.FlagPassThru())
			{
				if(Task.result)
					Task.PassAllResult(Task.result);
				return CACHE_PROCESS_OK;
			}

			// ATTN: if failed, node always purged
			if( Task.result && 
					((nodeSizeLimit > 0 && Task.result->DataLen() >= nodeSizeLimit) ||
					 (nodeRowsLimit > 0 && Task.result->TotalRows() >= nodeRowsLimit)))
			{
				log_error("key[%d] rows[%d] size[%d] exceed limit", 
						Task.IntKey(), Task.result->TotalRows(), Task.result->DataLen());
				iLimit = 1;
			}

			/* don't add empty node if Task back from blackhole */
			if(!iLimit && !Task.FlagBlackHole())
				cache_replace_result(Task);

			return cache_get_rb(Task);

		case DRequest::Insert:	//没有回读则必定是multirow,新数据附在原有数据后面
			if(Task.FlagBlackHole())
				return cache_nodb_insert(Task);

			if(insertOrder==INSERT_ORDER_PURGE)
			{
				cache_purge_data(Task);
				return CACHE_PROCESS_OK;
			}
			return cache_sync_insert(Task);

		case DRequest::Update:
			if(Task.FlagBlackHole())
				return cache_nodb_update(Task);

			if(insertOrder==INSERT_ORDER_PURGE && Task.resultInfo.AffectedRows() > 0)
			{
				cache_purge_data(Task);
				return CACHE_PROCESS_OK;
			}

			return cache_sync_update(Task);

		case DRequest::Delete:
			if(Task.FlagBlackHole())
				return cache_nodb_delete(Task);
			return cache_sync_delete(Task);

		case DRequest::Replace: 
			if(Task.FlagBlackHole())
				return cache_nodb_replace(Task);
			return cache_sync_replace(Task);

		case DRequest::SvrAdmin:
			if (Task.requestInfo.AdminCode() == DRequest::ServerAdminCmd::Migrate)
			{
					const CFieldValue* condition = Task.RequestCondition();
					const CValue* key = condition->FieldValue(0);
					CNode stNode = Cache.CacheFindAutoChoseHash(key->bin.ptr);
					Cache.PurgeNodeEverything(key->bin.ptr, stNode);
					log_debug("should purgenode everything");
					keyRoute->KeyMigrated(key->bin.ptr);
					delete(Task.RequestOperation());
					Task.SetRequestOperation(NULL);
					return CACHE_PROCESS_OK;
			}
			else
			{
					Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"invalid cmd from helper");
			}

		default:
			Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"invalid cmd from helper");
	}//end of switch

	return CACHE_PROCESS_ERROR;
}

/*
 * Function		: cache_process_fullmode
 * Description	: 处理incoming task
 * Input			: Task			请求信息
 * Output		: Task			返回信息
 * Return		: 0 			成功
 *				: -1			失败
 */
TCacheResult CCacheProcess::cache_process_nodb(CTaskRequest &Task)
{
	// nodb mode always blackhole-d
	Task.MarkAsBlackHole();
	Task.RenewTimestamp();
	szErrMsg[0] = 0;

	/* 取命令字 */
	int iCmd = Task.RequestCode();
	switch(iCmd){
		case DRequest::Get:
			if(Task.CountOnly() && (Task.requestInfo.LimitStart() || Task.requestInfo.LimitCount()))
			{
				Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"There's nothing to limit because no fields required");
				return CACHE_PROCESS_ERROR;
			}
			++statGetCount;
			Task.SetResultHitFlag(HIT_INIT);
			return cache_get_data(Task); 

		case DRequest::Insert:
			++ statInsertCount;
			return cache_nodb_insert(Task);

		case DRequest::Update:
			++ statUpdateCount;
			return cache_nodb_update(Task);

		case DRequest::Delete:
			++ statDeleteCount;
			return cache_nodb_delete(Task);

		case DRequest::Purge:
			//删除指定key在cache中的数据
			++ statPurgeCount;
			return cache_purge_data(Task);

		case DRequest::Flush:
			return CACHE_PROCESS_OK;

		case DRequest::Replace: //如果是淘汰的数据，不作处理
			++ statUpdateCount;
			// 限制key字段作为唯一字段才能使用replace命令
			if( !(Task.TableDefinition()->KeyPartOfUniqField()) || Task.TableDefinition()->HasAutoIncrement()){
				Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"replace cmd require key fields part of uniq-fields and no auto-increment field");
				return CACHE_PROCESS_ERROR;
			}
			return cache_nodb_replace(Task);

		case DRequest::SvrAdmin:
			return cache_process_admin(Task);

		default:
			Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"invalid cmd from client");
			log_notice("invalid cmd[%d] from client", iCmd);
			break;
	}//end of switch

	return CACHE_PROCESS_ERROR;
}

TCacheResult CCacheProcess::cache_flush_reply(CTaskRequest &Task)
{
	szErrMsg[0] = '\0';

	int iCmd = Task.RequestCode();
	switch(iCmd){
		case DRequest::Replace: //如果是淘汰的数据，不作处理
			return CACHE_PROCESS_OK;
		default:
			Task.SetError(-EC_BAD_COMMAND,CACHE_SVC,"invalid cmd from helper");
	}//end of switch

	return CACHE_PROCESS_ERROR;
}

/*
 * Function             : cache_process_error
 * Description  : ′|àítask from helper error
 * Input                        : Task                  ???óD??￠
 * Output               : Task                  ·μ??D??￠
 * Return               : 0                     3é1|
 *                              : -1                    ê§°ü
 */
TCacheResult CCacheProcess::cache_process_error(CTaskRequest &Task)
{
	// execute timeout
	szErrMsg[0] = '\0';

	switch(Task.RequestCode()){	
		case DRequest::Insert:
			if(lossyMode==true && Task.ResultCode()==-ER_DUP_ENTRY) {
				// upstream is un-trusted
				Task.RenewTimestamp();
				return cache_sync_insert(Task);
			}
			// FALLTHROUGH
		case DRequest::Delete:
			switch(Task.ResultCode())
			{
				case -EC_UPSTREAM_ERROR:
				case -CR_SERVER_LOST:
					if(updateMode == MODE_SYNC)
					{
						log_notice("SQL execute result unknown, purge data");
						cache_purge_data(Task);
					} else {
						log_crit("SQL execute result unknown, data may be corrupted");
					}
					break;
			}
			break;

		case DRequest::Update:
			switch(Task.ResultCode())
			{
				case -ER_DUP_ENTRY:
					if(lossyMode==true) {
						// upstream is un-trusted
						Task.RenewTimestamp();
						return cache_sync_update(Task);
					}
					// FALLTHROUGH
				case -EC_UPSTREAM_ERROR:
				case -CR_SERVER_LOST:
					if(updateMode == MODE_SYNC)
					{
						log_notice("SQL execute result unknown, purge data");
						cache_purge_data(Task);
					}
					// must be cache miss
					break;
			}
			break;
	}

	return CACHE_PROCESS_ERROR;
}

//----------------------------------------------------------------------------
