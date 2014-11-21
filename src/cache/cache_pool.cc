#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "bin_malloc.h"
#include "namespace.h"
#include "cache_pool.h"
#include "data_chunk.h"
#include "empty_filter.h"
#include "task_request.h"
#include "ttc_global.h"
#include "RelativeHourCalculator.h"


extern CTableDefinition *gTableDef[];
extern int hashChanging;
extern int targetNewHash;

TTC_USING_NAMESPACE

CCachePool::CCachePool(CPurgeNodeNotifier *pn):
    _purge_notifier(pn),
    _stRow(gTableDef[0])
{
    memset(&_cacheInfo, 0x00, sizeof(CacheInfo));

    _hash      = 0;
    _ngInfo    = 0;
    _feature   = 0;
    _nodeIndex = 0;
    _raw_data = 0;

    memset(_errmsg, 0, sizeof(_errmsg));
    _need_set_integrity = 0;
    _need_purge_node_count = 0;

    _delay_purge_timerlist = NULL;
    firstMarkerTime = lastMarkerTime = 0;
    emptyLimit = 0;
    disableTryPurge = 0;
    survival_hour = statmgr.GetSample(DATA_SURVIVAL_HOUR_STAT);
}

CCachePool::~CCachePool()
{
    _hash->Destroy();
    _ngInfo->Destroy();
    _feature->Destroy();
    _nodeIndex->Destroy();
    DELETE(_raw_data);

    /* 运行到这里，说明程序是正常stop的，设置共享内存完整性标记 */
    if(_need_set_integrity)
    {
        log_notice("Share Memory Integrity... ok");
        CBinMalloc::Instance()->SetShareMemoryIntegrity(1);
    }
}

/* 检查lru链表是否cross-link了，一旦发生这种情况，没法处理了 :( */
static inline int check_cross_linked_lru(CNode node)
{
    CNode v = node.Prev();

    if(v == node)
    {
        log_crit("BUG: cross-linked lru list");
        return -1;
    }
    return 0;
}

/* 验证cacheInfo合法性, 避免出现意外 */
int CCachePool::VerifyCacheInfo(CacheInfo *info)
{
    if(INVALID_HANDLE != 0UL)
    {
        snprintf(_errmsg, sizeof(_errmsg), "PANIC: invalid handle must be 0UL");
        return -1;
    }

    if(INVALID_NODE_ID != (NODE_ID_T)(-1))
    {
        snprintf(_errmsg, sizeof(_errmsg),
                "PANIC: invalid node id must be %u, but it is %u now",
                (NODE_ID_T)(-1), INVALID_NODE_ID);
        return -1;
    }

    if(info->version != 4)
    {
        snprintf(_errmsg, sizeof(_errmsg),
                "only support cache version >= 4");
        return -1;
    }

    /* 系统可工作的最小内存 */
    /* 1. emptyFilter = 0  Min=64M */
    /* 2. emptyFilter = 1  Min=256M, 初步按照1.5G用户来计算 */

    if(info->emptyFilter)
    {
        if(info->ipcMemSize < (256UL << 20)) 
        {
            snprintf(_errmsg, sizeof(_errmsg),
                    "Empty-Node Filter function need min 256M mem");
            return -1;
        }
    }

    if(info->ipcMemSize < (64UL << 20))
    {
        snprintf(_errmsg, sizeof(_errmsg),
                "too small mem size, need min 64M mem");
        return -1;
    }


    /* size check on 32bits platform*/	
    if(sizeof(long) == 4)
    {
        if(info->ipcMemSize >= UINT_MAX)
        {
            snprintf(_errmsg, sizeof(_errmsg),
                    "cache size "UINT64FMT"exceed 4G, Please upgrade to 64 bit version",
                    info->ipcMemSize);
            return -1;
        }
    }

    /* support max 64G memory size*/
    if(info->ipcMemSize > (64ULL << 30))
    {
        snprintf(_errmsg, sizeof(_errmsg),
                "cache size exceed 64G, unsupported");
        return -1;
    }

    return 0;
}

int CCachePool::CacheOpen(CacheInfo *info)
{
TRY_CACHE_INIT_AGAIN:
    if(info->readOnly==0)
    {
        if(VerifyCacheInfo(info) != 0)
            return -1;
        memcpy((char *)&_cacheInfo, info, sizeof(CacheInfo));
    } 
    else
    {
        memset((char *)&_cacheInfo, 0, sizeof(CacheInfo));
        _cacheInfo.readOnly = 1;
        _cacheInfo.keySize = info->keySize;
        _cacheInfo.ipcMemKey = info->ipcMemKey;
    }

    //初始化统计对象
    statCacheSize = statmgr.GetItemU32(TTC_CACHE_SIZE);
    statCacheKey  = statmgr.GetItemU32(TTC_CACHE_KEY);
    statCacheVersion = statmgr.GetItem(TTC_CACHE_VERSION);
    statUpdateMode = statmgr.GetItemU32(TTC_UPDATE_MODE);
    statEmptyFilter = statmgr.GetItemU32(TTC_EMPTY_FILTER);
    statHashSize  = statmgr.GetItemU32(TTC_BUCKET_TOTAL);
    statFreeBucket  = statmgr.GetItemU32(TTC_FREE_BUCKET);
    statDirtyEldest  = statmgr.GetItemU32(TTC_DIRTY_ELDEST);
    statDirtyAge  = statmgr.GetItemU32(TTC_DIRTY_AGE);
    statTryPurgeCount = statmgr.GetSample(TRY_PURGE_COUNT);
    statTryPurgeNodes = statmgr.GetItemU32(TRY_PURGE_NODES);
    statLastPurgeNodeModTime = statmgr.GetItemU32(LAST_PURGE_NODE_MOD_TIME);
    statDataExistTime = statmgr.GetItemU32(DATA_EXIST_TIME);

    //打开共享内存
    if(_shm.Open(_cacheInfo.ipcMemKey) > 0)
    {
        //共享内存已存在

        if(_cacheInfo.createOnly)
        {
            snprintf(_errmsg, sizeof(_errmsg), "m_shm already exist");
            return -1;
        }

        if(_cacheInfo.readOnly==0 && _shm.Lock() != 0)
        {
            snprintf(_errmsg, sizeof(_errmsg), "m_shm.Lock() failed");
            return -1;
        }

        if(_shm.Attach(_cacheInfo.readOnly)==NULL)
        {
            snprintf(_errmsg, sizeof(_errmsg), "m_shm.Attach() failed");
            return -1;
        }

        //底层分配器
        if(CBinMalloc::Instance()->Attach(_shm.Ptr(), _shm.Size()) != 0)
        {
            snprintf(_errmsg, 
                    sizeof(_errmsg), 
                    "binmalloc attach failed: %s", 
                    M_ERROR());
            return -1;
        }

        //内存版本检测, 目前因为底层分配器的缘故，只支持version >= 4的版本
        _cacheInfo.version = CBinMalloc::Instance()->DetectVersion();
        if(_cacheInfo.version != 4)
        {
            snprintf(_errmsg, sizeof(_errmsg), "unsupport version, %d", _cacheInfo.version);
            return -1;
        }

        /* 检查共享内存完整性，通过*/
        if(CBinMalloc::Instance()->ShareMemoryIntegrity())
        {
            log_notice("Share Memory Integrity Check.... ok");
            /* 
             * 设置共享内存不完整标记
             *
             * 这样可以在程序coredump引起内存混乱时，再次重启后ttc能发现内存已经写乱了。
             */
            if(_cacheInfo.readOnly==0)
            {
                _need_set_integrity = 1;
                CBinMalloc::Instance()->SetShareMemoryIntegrity(0);
            }
        }
        /* 不通过 */
        else
        {
            log_warning("Share Memory Integrity Check... failed");

            if(_cacheInfo.autoDeleteDirtyShm)
            {
                if(_cacheInfo.readOnly==1)
                {
                    log_error("ReadOnly Share Memory is Confuse");
                    return -1;
                }

                /* 删除共享内存，重新启动cache初始化流程 */
                if(_shm.Delete() < 0)
                {
                    log_error("Auto Delete Share Memory failed: %m");
                    return -1;
                }

                log_notice("Auto Delete Share Memory Success, Try Rebuild");

                _shm.Unlock();

                CBinMalloc::Destroy();

                /* 重新初始化 */
                goto TRY_CACHE_INIT_AGAIN;
            }
        }
    }

    //共享内存不存在，需要创建
    else
    {
        //只读，失败
        if(_cacheInfo.readOnly)
        {
            snprintf(_errmsg, sizeof(_errmsg), "readonly m_shm non-exists");
            return -1;
        }

        //创建
        if(_shm.Create(_cacheInfo.ipcMemKey, _cacheInfo.ipcMemSize) <= 0)
        {
            if(errno == EACCES || errno == EEXIST)
                snprintf(_errmsg, sizeof(_errmsg), "m_shm exists but unwritable");
            else
                snprintf(_errmsg, sizeof(_errmsg), "create m_shm failed: %m");
            return -1;
        }

        if(_shm.Lock() != 0)
        {
            snprintf(_errmsg, sizeof(_errmsg), "m_shm.Lock() failed");
            return -1;
        }

        if(_shm.Attach()==NULL)
        {
            snprintf(_errmsg, sizeof(_errmsg), "m_shm.Attach() failed");
            return -1;
        }

        //底层分配器初始化
        if(CBinMalloc::Instance()->Init(_shm.Ptr(), _shm.Size()) != 0)
        {
            snprintf(_errmsg, 
                    sizeof(_errmsg), 
                    "binmalloc init failed: %s", 
                    M_ERROR());
            return -1;
        }

        /* 
         * 设置共享内存不完整标记
         */
        _need_set_integrity = 1;
        CBinMalloc::Instance()->SetShareMemoryIntegrity(0);

    }

    /* statistic */
    statCacheSize = _cacheInfo.ipcMemSize;
    statCacheKey  = _cacheInfo.ipcMemKey;
    statCacheVersion = _cacheInfo.version;
    statUpdateMode = _cacheInfo.syncUpdate;
    statEmptyFilter = _cacheInfo.emptyFilter;
    /*add for AutoPurge alert* */
    _raw_data = new CRawData(CBinMalloc::Instance(),1);
    /*set minchunksize*/
    CBinMalloc::Instance()->SetMinChunkSize(CTTCGlobal::_min_chunk_size);


    //attention: invoke AppStorageOpen() must after CBinMalloc init() or attach().
    return AppStorageOpen();
}

int CCachePool::AppStorageOpen()
{
    APP_STORAGE_T* storage = M_POINTER(APP_STORAGE_T, CBinMalloc::Instance()->GetReserveZone());
    if(!storage)
    {
        snprintf(_errmsg, 
                sizeof(_errmsg), 
                "get reserve zone from binmalloc failed: %s", 
                M_ERROR());


        return -1;
    }

    return TTCMemOpen(storage);
}

int CCachePool::TTCMemOpen(APP_STORAGE_T *storage)
{
    if(storage->NeedFormat())
    {
        log_debug("starting init ttc mem");
        return TTCMemInit(storage);
    }

    return TTCMemAttach(storage);

}

/* hash size = 1% total memory size */
/* return hash bucket num*/

uint32_t CCachePool::HashBucketNum(uint64_t size)
{
    int h = (uint32_t)(size/100 - 16)/sizeof(NODE_ID_T);
    h = (h/9)*9;
    return h;
}

int CCachePool::TTCMemInit(APP_STORAGE_T *storage)
{
    _feature = CFeature::Instance();
    if(!_feature || _feature->Init(MIN_FEATURES))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "init feature failed, %s",
                _feature->Error());
        return -1;
    }

    if(storage->Format(_feature->Handle()))
    {
        snprintf(_errmsg, sizeof(_errmsg), "format storage failed");
        return -1;
    }

    /* Node-Index*/
    _nodeIndex = CNodeIndex::Instance();
    if(!_nodeIndex || _nodeIndex->Init(_cacheInfo.ipcMemSize))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "init node-index failed, %s",
                _nodeIndex->Error());
        return -1;
    }

    /* Hash-Bucket */
    _hash = CHash::Instance();
    if(!_hash || _hash->Init(HashBucketNum(_cacheInfo.ipcMemSize), _cacheInfo.keySize))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "init hash-bucket failed, %s",
                _hash->Error());
        return -1;
    }
    statHashSize = _hash->HashSize(); 
    statFreeBucket = _hash->FreeBucket();

    /* NG-Info */
    _ngInfo = CNGInfo::Instance();
    if(!_ngInfo || _ngInfo->Init())
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "init ng-info failed, %s",
                _ngInfo ->Error());
        return -1;
    }

    /* insert features*/
    if(_feature->AddFeature(NODE_INDEX, _nodeIndex->Handle()))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "add node-index feature failed, %s",
                _feature->Error());
        return -1;
    }

    if(_feature->AddFeature(HASH_BUCKET, _hash->Handle()))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "add hash-bucket feature failed, %s",
                _feature->Error());
        return -1;
    }

    if(_feature->AddFeature(NODE_GROUP, _ngInfo->Handle()))
    {
        snprintf(_errmsg, sizeof(_errmsg), 
                "add node-group feature failed, %s",
                _feature->Error());
        return -1;
    }

    /* Empty-Node Filter*/
    if(_cacheInfo.emptyFilter)
    {
        CEmptyNodeFilter *p = CEmptyNodeFilter::Instance();
        if(!p || p->Init())
        {
            snprintf(_errmsg, sizeof(_errmsg),
                    "start Empty-Node Filter failed, %s",
                    p->Error());
            return -1;
        }

        if(_feature->AddFeature(EMPTY_FILTER, p->Handle()))
        {
            snprintf(_errmsg, sizeof(_errmsg), 
                    "add empty-filter feature failed, %s",
                    _feature->Error());
            return -1;
        }

    }

    statDirtyEldest = 0;
    statDirtyAge  = 0;

    return 0;
}

int CCachePool::TTCMemAttach(APP_STORAGE_T *storage)
{

    _feature = CFeature::Instance();
    if(!_feature || _feature->Attach(storage->as_extend_info))
    {
        snprintf(_errmsg, sizeof(_errmsg), "%s", _feature->Error());
        return -1;
    }

    /*hash-bucket*/
    FEATURE_INFO_T* p = _feature->GetFeatureById(HASH_BUCKET);
    if(!p){
        snprintf(_errmsg, sizeof(_errmsg), "not found hash-bucket feature");
        return -1;
    }
    _hash = CHash::Instance();
    if(!_hash || _hash->Attach(p->fi_handle))
    {
        snprintf(_errmsg, sizeof(_errmsg), "%s", _hash->Error());
        return -1;
    }
    statHashSize = _hash->HashSize(); 
    statFreeBucket = _hash->FreeBucket();

    /*node-index*/
    p = _feature->GetFeatureById(NODE_INDEX);
    if(!p){
        snprintf(_errmsg, sizeof(_errmsg), "not found node-index feature");
        return -1;
    }
    _nodeIndex = CNodeIndex::Instance();
    if(!_nodeIndex || _nodeIndex->Attach(p->fi_handle))
    {
        snprintf(_errmsg, sizeof(_errmsg), "%s", _nodeIndex->Error());
        return -1;
    }

    /*ng-info*/
    p = _feature->GetFeatureById(NODE_GROUP);
    if(!p){
        snprintf(_errmsg, sizeof(_errmsg), "not found ng-info feature");
        return -1;
    }
    _ngInfo = CNGInfo::Instance();	
    if(!_ngInfo || _ngInfo->Attach(p->fi_handle))
    {
        snprintf(_errmsg, sizeof(_errmsg), "%s", _ngInfo->Error());
        return -1;
    }

    CNode stLastTime = LastTimeMarker();
    CNode stFirstTime = FirstTimeMarker();
    if(!(!stLastTime) && !(!stFirstTime)){
        statDirtyEldest = stLastTime.Time();
        statDirtyAge  = stFirstTime.Time() - stLastTime.Time();
    }

    //TODO tableinfo
    return 0;
}

// Sync the empty node statstics
int CCachePool::InitEmptyNodeList(void)
{
    if(_ngInfo->EmptyStartupMode() == CNGInfo::ATTACHED)
    {
        // iterate through empty lru list
        // re-counting the total empty lru statstics

        // empty lru header
        int count = 0;
        CNode header = _ngInfo->EmptyNodeHead();
        CNode pos;

        for(pos = header.Prev(); pos != header; pos = pos.Prev())
        { 
            /* check whether cross-linked */
            if(check_cross_linked_lru(pos) < 0)
                break;
            count++;
        }
        _ngInfo->IncEmptyNode(count);
        log_info("found %u empty nodes inside empty lru list", count);
    }
    return 0;
}

// migrate empty node from clean list to empty list
int CCachePool::UpgradeEmptyNodeList(void)
{
    if(_ngInfo->EmptyStartupMode() != CNGInfo::CREATED)
    {
        int count = 0;
        CNode header = _ngInfo->CleanNodeHead();
        CNode next;

        for(CNode pos = header.Prev(); pos != header; pos = next)
        { 
            /* check whether cross-linked */
            if(check_cross_linked_lru(pos) < 0)
                break;
            next = pos.Prev();

            if(NodeRowsCount(pos) == 0) {
                _ngInfo->RemoveFromLru(pos);
                _ngInfo->Insert2EmptyLru(pos);
                count++;
            }
        }
        _ngInfo->IncEmptyNode(count);
        log_info("found %u empty nodes inside clean lru list, move to empty lru", count);
    }

    return 0;
}

// migrate empty node from empty list to clean list
int CCachePool::MergeEmptyNodeList(void)
{
    if(_ngInfo->EmptyStartupMode() != CNGInfo::CREATED)
    {
        int count = 0;
        CNode header = _ngInfo->EmptyNodeHead();
        CNode next;

        for(CNode pos = header.Prev(); pos != header; pos = next)
        { 
            /* check whether cross-linked */
            if(check_cross_linked_lru(pos) < 0)
                break;
            next = pos.Prev();

            _ngInfo->RemoveFromLru(pos);
            _ngInfo->Insert2CleanLru(pos);
            count++;
        }
        log_info("found %u empty nodes, move to clean lru", count);
    }

    return 0;
}

// prune all empty nodes
int CCachePool::PruneEmptyNodeList(void)
{
    if(_ngInfo->EmptyStartupMode() == CNGInfo::ATTACHED)
    {
        int count = 0;
        CNode header = _ngInfo->EmptyNodeHead();
        CNode next;

        for(CNode pos = header.Prev(); pos != header; pos = next)
        { 
            /* check whether cross-linked */
            if(check_cross_linked_lru(pos) < 0)
                break;
            next = pos.Prev();

            count++;
            PurgeNodeEverything(pos);
        }

        log_info("fullmode: total %u empty nodes purged", count);
    }

    return 0;
}

int CCachePool::ShrinkEmptyNodeList(void)
{
    if(emptyLimit && _ngInfo->EmptyCount() > emptyLimit)
    {
        //bug fix recalc empty
        int togo = _ngInfo->EmptyCount() - emptyLimit;
        int count = 0;
        CNode header = _ngInfo->EmptyNodeHead();
        CNode next;

        for(CNode pos = header.Prev(); count < togo && pos != header; pos = next)
        { 
            /* check whether cross-linked */
            if(check_cross_linked_lru(pos) < 0)
                break;

            next = pos.Prev();

            PurgeNodeEverything(pos);
            _ngInfo->IncEmptyNode(-1);
            count++;
        }
        log_info("shrink empty lru, %u empty nodes purged", count);
    }

    return 0;
}

int CCachePool::PurgeSingleEmptyNode(void)
{
    CNode header = _ngInfo->EmptyNodeHead();
    CNode pos = header.Prev();

    if(pos != header)
    {
        /* check whether cross-linked */
        if(check_cross_linked_lru(pos) < 0)
            return -1;

        log_debug("empty node execeed limit, purge node %u", pos.NodeID());
        PurgeNodeEverything(pos);
        _ngInfo->IncEmptyNode(-1);
    }

    return 0;
}

/* insert node to hash bucket*/
int CCachePool::Insert2Hash(const char *key, CNode node)
{
    HASH_ID_T hashslot;

    if(targetNewHash)
    {
        hashslot = _hash->NewHashSlot(key);
    }else
    {
        hashslot = _hash->HashSlot(key);
    }

    if(_hash->Hash2Node(hashslot) == INVALID_NODE_ID)
    {
	_hash->IncFreeBucket(-1);
	--statFreeBucket;
    }

    _hash->IncNodeCnt(1);

    node.NextNodeID() = _hash->Hash2Node(hashslot);
    _hash->Hash2Node(hashslot) = node.NodeID();

    return 0;
}

int CCachePool::RemoveFromHashBase(const char *key, CNode remove_node, int newhash)
{
    HASH_ID_T hash_slot;

    if(newhash){
        hash_slot = _hash->NewHashSlot(key);
    }
    else{
	hash_slot = _hash->HashSlot(key);
    }

    NODE_ID_T node_id   = _hash->Hash2Node(hash_slot);

    /* hash miss */
    if(node_id == INVALID_NODE_ID)
	return 0;

    /* found in hash head */
    if(node_id == remove_node.NodeID())
    {
	_hash->Hash2Node(hash_slot) = remove_node.NextNodeID();

	// stat
	if(_hash->Hash2Node(hash_slot) == INVALID_NODE_ID)
	{
	    _hash->IncFreeBucket(1);
	    ++statFreeBucket;
	}

	_hash->IncNodeCnt(-1);
	return 0;
    }

    CNode prev = I_SEARCH(node_id);
    CNode next = I_SEARCH(prev.NextNodeID());

    while(!(!next) && next.NodeID() != remove_node.NodeID())
    {
	prev = next;
	next = I_SEARCH(next.NextNodeID());
    }

    /* found */
    if(!(!next))
    {
	prev.NextNodeID() = next.NextNodeID();
	_hash->IncNodeCnt(-1);
    }else
    {       
	log_error("RemoveFromHash failed, node-id [%d] not found in slot %u ",
		remove_node.NodeID(), hash_slot);
	return -1;
    }

    return 0;
}

int CCachePool::RemoveFromHash(const char *key, CNode remove_node)
{
    if(hashChanging)
    {
        RemoveFromHashBase(key, remove_node, 1);
	RemoveFromHashBase(key, remove_node, 0);
    }else
    {
        if(targetNewHash)
	    RemoveFromHashBase(key, remove_node, 1);
	else
	    RemoveFromHashBase(key, remove_node, 0);
    }

    return 0;
}

int CCachePool::MoveToNewHash(const char * key, CNode node)
{
    RemoveFromHash(key, node);
    Insert2Hash(key, node);
    return 0;
}


inline int CCachePool::KeyCmp(const char *key, const char *other)
{
    int len = _cacheInfo.keySize == 0 ? 
        (*(unsigned char *)key + 1) : _cacheInfo.keySize;

    return memcmp(key, other, len);
}

CNode CCachePool::CacheFindAutoChoseHash(const char *key)
{
		int oldhash = 0;
		int newhash = 1;
		CNode stNode;

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

				stNode = CacheFind(key, oldhash);
				if(!stNode)
				{
						stNode = CacheFind(key, newhash);
				}else
				{
						MoveToNewHash(key, stNode);
				}
		}else
		{
				if(targetNewHash)
				{
						stNode = CacheFind(key, 1);
				}else
				{
						stNode = CacheFind(key, 0);
				}
		}
		return stNode;

}

CNode CCachePool::CacheFind(const char *key, int newhash)
{
    HASH_ID_T hash_slot;

    if(newhash)
    {
	hash_slot = _hash->NewHashSlot(key);
    }else
    {
        hash_slot = _hash->HashSlot(key);
    }

    NODE_ID_T node_id   = _hash->Hash2Node(hash_slot);

    /* not found */
    if(node_id == INVALID_NODE_ID)
        return CNode();

    CNode iter = I_SEARCH(node_id);
    while(!(!iter))
    {
        /* 
         * 规避bug，purge无效节点 :(
         */
        if(iter.VDHandle() == INVALID_HANDLE)
        {
            log_warning("node[%u]'s handle is invalid", iter.NodeID());
            CNode purge_node = iter;
            iter = I_SEARCH(iter.NextNodeID());
            PurgeNode(key, purge_node);
            continue;
        }

        CDataChunk *data_chunk = M_POINTER(CDataChunk, iter.VDHandle());
        if(NULL == data_chunk)
        {
            log_warning("node[%u]'s handle is invalid", iter.NodeID());
            CNode purge_node = iter;
            iter = I_SEARCH(iter.NextNodeID());
            PurgeNode(key, purge_node);
            continue;
        }

        if(NULL == data_chunk->Key())
        {
            log_warning("node[%u]'s handle is invalid, decode key failed", iter.NodeID());
            CNode purge_node = iter;
            iter = I_SEARCH(iter.NextNodeID());
            PurgeNode(key, purge_node);
            continue;
        }

        /* EQ */
        if(KeyCmp(key, data_chunk->Key()) == 0)
        {
            log_debug("found node[%u]", iter.NodeID());
            return iter;
        }

        iter = I_SEARCH(iter.NextNodeID());
    }

    /* not found*/
    return CNode();
}

unsigned int CCachePool::FirstTimeMarkerTime(void)
{
    if(firstMarkerTime == 0)
    {
        CNode marker = FirstTimeMarker();
        firstMarkerTime = !marker ? 0 : marker.Time();
    }
    return firstMarkerTime;
}

unsigned int CCachePool::LastTimeMarkerTime(void)
{
    if(lastMarkerTime == 0)
    {
        CNode marker = LastTimeMarker();
        lastMarkerTime = !marker ? 0 : marker.Time();
    }
    return lastMarkerTime;
}

/* insert a time-marker to dirty lru list*/
int CCachePool::InsertTimeMarker(unsigned int t)
{
    CNode tm_node = _ngInfo->AllocateNode();
    if(!tm_node)
    {
	    log_debug("no mem allocate timemarker, purge 10 clean node");
	    /* prepurge clean node for cache is full */
	    PrePurgeNodes(10, CNode());
	    tm_node = _ngInfo->AllocateNode();
	    if(!tm_node)
	    {
		    log_crit("can not allocate time marker for dirty lru");
		    return -1;
	    }
    }

    log_debug("insert time marker in dirty lru, time %u", t);
    tm_node.NextNodeID() = TIME_MARKER_NEXT_NODE_ID;
    tm_node.VDHandle()   = t;

    _ngInfo->Insert2DirtyLru(tm_node);

    //stat
    firstMarkerTime = t;

    /*in case lastMarkerTime not set*/
    if(lastMarkerTime == 0)
        LastTimeMarkerTime();

    statDirtyAge  = firstMarkerTime - lastMarkerTime;
    statDirtyEldest = lastMarkerTime;

    return 0;
}

/* -1: not a time marker
 * -2: this the only time marker

 */
int CCachePool::RemoveTimeMarker(CNode node)
{
    /* is not timermarker node */
    if(!IsTimeMarker(node))
        return -1;

    _ngInfo->RemoveFromLru(node);
    _ngInfo->ReleaseNode(node);

    //stat	
    CNode stLastTime = LastTimeMarker();
    if(!stLastTime) {
        lastMarkerTime = firstMarkerTime;
    } else {
        lastMarkerTime = stLastTime.Time();
    }

    statDirtyAge  = firstMarkerTime - lastMarkerTime;
    statDirtyEldest = lastMarkerTime;
    return  0;
}

/* prev <- dirtyhead */
CNode CCachePool::LastTimeMarker() const
{
    CNode pos, dirtyHeader = _ngInfo->DirtyNodeHead();
    NODE_LIST_FOR_EACH_RVS(pos, dirtyHeader)
    {
        if(pos.NextNodeID()  == TIME_MARKER_NEXT_NODE_ID)
            return pos;
    }

    return CNode();
}

/* dirtyhead -> next */
CNode CCachePool::FirstTimeMarker() const
{
    CNode pos, dirtyHeader = _ngInfo->DirtyNodeHead();

    NODE_LIST_FOR_EACH(pos, dirtyHeader)
    {
        if(pos.NextNodeID()  == TIME_MARKER_NEXT_NODE_ID)
            return pos;
    }

    return CNode();
}

/* dirty lru list head */
CNode CCachePool::DirtyLruHead() const
{
    return _ngInfo->DirtyNodeHead();
}

/* clean lru list head */
CNode CCachePool::CleanLruHead() const
{
    return _ngInfo->CleanNodeHead();
}

/* empty lru list head */
CNode CCachePool::EmptyLruHead() const
{
    return _ngInfo->EmptyNodeHead();
}

int CCachePool::IsTimeMarker(CNode node) const
{
    return node.NextNodeID() == TIME_MARKER_NEXT_NODE_ID ;
}

int CCachePool::TryPurgeSize(size_t size, CNode reserve, unsigned max_purge_count)
{
    log_debug("start TryPurgeSize");

    if(disableTryPurge) {
        static int alert_count = 0;
        if (!alert_count++)
        {
            log_alert("memory overflow, auto purge disabled");
        }
        return -1;
    }
    /*if have pre purge, purge node and continue*/
    /* prepurge should not purge reserved node in TryPurgeSize */
    PrePurgeNodes(CTTCGlobal::_pre_purge_nodes, reserve);

    unsigned real_try_purge_count=0;

    /* clean lru header */
    CNode clean_header = CleanLruHead();

    CNode pos = clean_header.Prev();

    for(unsigned iter=0; iter<max_purge_count && !(!pos) && pos != clean_header; ++iter)
    { 
        CNode purge_node = pos;

        if(TotalUsedNode() < 10)
            break;

        /* check whether cross-linked */
        if(check_cross_linked_lru(pos) < 0)
            break;

        pos = pos.Prev();

        if(purge_node == reserve)
        {
            continue;
        }

        if(purge_node.VDHandle() == INVALID_HANDLE)
        {
            log_warning("node[%u]'s handle is invalid", purge_node.NodeID());
            continue;
        }

        /* ask for data-chunk's size */
        CDataChunk *data_chunk = M_POINTER(CDataChunk, purge_node.VDHandle());
        if(NULL == data_chunk)
        {
            log_warning("node[%u] handle is invalid, attach CDataChunk failed", purge_node.NodeID());
            continue;
        }

        unsigned combine_size= data_chunk->AskForDestroySize(CBinMalloc::Instance());
        log_debug("need_size=%u, combine-size=%u, node-size=%u", 
                (unsigned)size, combine_size, data_chunk->NodeSize());

        if(combine_size >= size)
        {
            /* stat total rows */
            IncTotalRow(0LL-NodeRowsCount(purge_node));
            CheckAndPurgeNodeEverything(purge_node);
            _need_purge_node_count = iter;
            ++statTryPurgeNodes;
            return 0;
        }

        ++real_try_purge_count;
    }

    _need_purge_node_count = real_try_purge_count;
    return -1;
}

int CCachePool::PurgeNode(const char *key, CNode purge_node)
{
    /* HB */
    if(_purge_notifier)
        _purge_notifier->PurgeNodeNotify(key, purge_node);

    /*1. Remove from hash */
    RemoveFromHash(key, purge_node);

    /*2. Remove from LRU */
    _ngInfo->RemoveFromLru(purge_node);

    /*3. Release node, it can auto remove from nodeIndex */
    _ngInfo->ReleaseNode(purge_node);

    return 0;
}

int CCachePool::PurgeNodeEverything(CNode purge_node)
{
    /* invalid node attribute */
    if(!(!purge_node) && purge_node.VDHandle() != INVALID_HANDLE)
    {
        CDataChunk * data_chunk = M_POINTER(CDataChunk, purge_node.VDHandle());
        if(NULL == data_chunk || NULL == data_chunk->Key())
        {
            log_error("node[%u]'s handle is invalid, can't attach and decode key", purge_node.NodeID());
            //TODO
            return -1;
        }
	uint32_t dwCreatetime = data_chunk->CreateTime();	
	uint32_t dwPurgeHour = RELATIVE_HOUR_CALCULATOR->GetRelativeHour();
	 log_debug("lru purge node,node[%u]'s createhour is %u, purgeHour is %u", purge_node.NodeID(), dwCreatetime, dwPurgeHour);
	 survival_hour.push((dwPurgeHour - dwCreatetime));
	 
        char key[256] = {0};
        /* decode key */
        memcpy(key, data_chunk->Key(), _cacheInfo.keySize > 0 ?
                _cacheInfo.keySize : 
                *(unsigned char *)(data_chunk->Key())+1);

        /* destroy data-chunk */
        data_chunk->Destroy(CBinMalloc::Instance());

        return PurgeNode(key, purge_node);
    }

    return 0;
}

uint32_t CCachePool::GetCmodtime(CNode* purge_node)
{
    uint32_t lastcmod = 0;
    uint32_t lastcmod_thisrow = 0;
    int iRet = _raw_data->Attach(purge_node->VDHandle(), gTableDef[0]->KeyFields()-1,
                        gTableDef[0]->KeyFormat(),-1,gTableDef[0]->LastcmodFieldId());
    if(iRet != 0){
        log_error("raw-data attach[handle:"UINT64FMT"] error: %d,%s",
                purge_node->VDHandle(), iRet, _raw_data->GetErrMsg());
        return(0);
    }

    unsigned int uiTotalRows = _raw_data->TotalRows();
    for(unsigned int i = 0; i <uiTotalRows; i++ )	//查找
    {
        if((iRet = _raw_data->GetLastcmod(_stRow,lastcmod_thisrow)) != 0){
            log_error("raw-data decode row error: %d,%s", iRet, _raw_data->GetErrMsg());
            return(0);
        }
        if (lastcmod_thisrow > lastcmod)
            lastcmod = lastcmod_thisrow;
    }
    return lastcmod;
}

//check if node's timestamp max than setting
//and PurgeNodeEverything
int CCachePool::CheckAndPurgeNodeEverything(CNode purge_node)
{
    int dataExistTime = statDataExistTime;
    if (dateExpireAlertTime)
    {
        struct timeval tm;
        gettimeofday(&tm,NULL);
        unsigned int lastnodecmodtime = GetCmodtime(&purge_node);
        if (lastnodecmodtime > statLastPurgeNodeModTime)
        {
            statLastPurgeNodeModTime = lastnodecmodtime;
            dataExistTime = (unsigned int)tm.tv_sec - statLastPurgeNodeModTime;
            statDataExistTime = dataExistTime;
        }
        if (statDataExistTime < dateExpireAlertTime)
        {
            static int alert_count = 0;
            if (!alert_count++)
            {
                log_alert("DataExistTime:%u is little than setting:%u",dataExistTime,dateExpireAlertTime);
            }
        }
            log_debug("dateExpireAlertTime:%d ,lastnodecmodtime:%d,timenow:%u",dateExpireAlertTime,lastnodecmodtime,(uint32_t)tm.tv_sec);
    }

    return PurgeNodeEverything(purge_node);
}
int CCachePool::PurgeNodeEverything(const char *key, CNode purge_node)
{
    CDataChunk * data_chunk = NULL;
    if(!(!purge_node) && purge_node.VDHandle() != INVALID_HANDLE)
    {
        data_chunk = M_POINTER(CDataChunk, purge_node.VDHandle());
        if(NULL == data_chunk)
        {
            log_error("node[%u]'s handle is invalid, can't attach data-chunk", purge_node.NodeID());
            return -1;
        }
	uint32_t dwCreatetime = data_chunk->CreateTime();	
	uint32_t dwPurgeHour = RELATIVE_HOUR_CALCULATOR->GetRelativeHour();
	 log_debug("black list purge node, node[%u]'s createhour is %u, purgeHour is %u", purge_node.NodeID(), dwCreatetime, dwPurgeHour);
	 survival_hour.push((dwPurgeHour - dwCreatetime));
        /* destroy data-chunk */
        data_chunk->Destroy(CBinMalloc::Instance());

    }

    if(!(!purge_node))
        return PurgeNode(key, purge_node);
    return 0;
}


/* allocate a new node by key */
CNode CCachePool::CacheAllocate(const char *key)
{
    CNode allocate_node = _ngInfo->AllocateNode();

    /* allocate failed */
    if(!allocate_node)
	return allocate_node;

    /*1. Insert to hash bucket */
    Insert2Hash(key, allocate_node);

    /*2. Insert to clean Lru list*/
    _ngInfo->Insert2CleanLru(allocate_node);

    return allocate_node;
}

extern int useNewHash;

/* purge key{data-chunk, hash, lru, node...} */
int CCachePool::CachePurge(const char *key)
{
    CNode purge_node;
    
    if(hashChanging)
    {
	purge_node = CacheFind(key, 0);
	if(!purge_node)
	{
	    purge_node = CacheFind(key, 1);
	    if(!purge_node)
	    {
		return 0;
	    }else
	    {
	        if(PurgeNodeEverything(key, purge_node) < 0)
		    return -1;
	    }
	}else
	{
	    if(PurgeNodeEverything(key, purge_node) < 0)
		return -1;
	}
    }else
    {
	if(targetNewHash)
	{
	    purge_node = CacheFind(key, 1);
	    if(!purge_node)
		return 0;
	    else
	    {
		if(PurgeNodeEverything(key, purge_node) < 0)
		    return -1;
	    }
	}else
	{
	    purge_node = CacheFind(key, 0);
	    if(!purge_node)
		return 0;
	    else
	    {
		if(PurgeNodeEverything(key, purge_node) < 0)
		    return -1;
	    }
	}
    }

    return 0;
}

void CCachePool::DelayPurgeNotify(const unsigned count)
{
    if(_need_purge_node_count == 0)
        return ;
    else
        statTryPurgeCount.push(_need_purge_node_count);

    unsigned purge_count = count < _need_purge_node_count ? count : _need_purge_node_count;
    unsigned real_purge_count = 0;

    log_debug("DelayPurgeNotify: total=%u, now=%u", _need_purge_node_count, purge_count);

    /* clean lru header */
    CNode clean_header = CleanLruHead();
    CNode pos = clean_header.Prev();

    while(purge_count-- > 0 && !(!pos) && pos != clean_header)
    {
        CNode purge_node = pos;
        check_cross_linked_lru(pos);
        pos = pos.Prev();

        /* stat total rows */
        IncTotalRow(0LL-NodeRowsCount(purge_node));

        CheckAndPurgeNodeEverything(purge_node);

        ++statTryPurgeNodes;
        ++real_purge_count;
    }

    _need_purge_node_count -= real_purge_count;

    /* 如果没有请求，重新调度delay purge任务 */
    if(_need_purge_node_count>0)
        AttachTimer(_delay_purge_timerlist);

    return;
}

int CCachePool::PrePurgeNodes(int purge_count, CNode reserve)
{
    int realpurged = 0;

    if(purge_count <= 0)
       return 0;
    else
       statTryPurgeCount.push(purge_count);

    /* clean lru header */
    CNode clean_header = CleanLruHead();
    CNode pos = clean_header.Prev();

    while(purge_count-- > 0 && !(!pos) && pos != clean_header)
    {
        CNode purge_node = pos;
        check_cross_linked_lru(pos);
        pos = pos.Prev();

	if(reserve == purge_node)
		continue;

        /* stat total rows */
        IncTotalRow(0LL-NodeRowsCount(purge_node));
        CheckAndPurgeNodeEverything(purge_node);
        ++statTryPurgeNodes;
	realpurged++;
    }
    return realpurged;;
}


int CCachePool::PurgeByTime(unsigned int oldesttime)
{
    return 0;
}

void CCachePool::StartDelayPurgeTask(CTimerList *timer)
{
    log_info("start delay-purge task");
    _delay_purge_timerlist = timer;
    AttachTimer(_delay_purge_timerlist);

    return;
}
void CCachePool::TimerNotify(void)
{
    log_debug("sched delay-purge task");

    DelayPurgeNotify();
}
