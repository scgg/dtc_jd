#ifndef __TTC_CACHE_POOL_H
#define __TTC_CACHE_POOL_H

#include <stddef.h>
#include "StatTTC.h"
#include "namespace.h"
#include "bin_malloc.h"
#include "shmem.h"
#include "global.h"
#include "node_list.h"
#include "node_index.h"
#include "node_group.h"
#include "feature.h"
#include "ng_info.h"
#include "hash.h"
#include "node.h"
#include "timerlist.h"
#include "data_chunk.h"

TTC_BEGIN_NAMESPACE

/* time-marker node in dirty lru list */
//#define TIME_MARKER_NEXT_NODE_ID INVALID_NODE_ID
#define TIME_MARKER_NEXT_NODE_ID (INVALID_NODE_ID-1)

/* cache基本信息 */
typedef struct _CacheInfo{
	int		ipcMemKey;		// 共享内存key
	uint64_t	ipcMemSize;		// 共享内存大小
	unsigned short	keySize;		// key大小
	unsigned char	version;		// 内存版本号
	unsigned char	syncUpdate:1; 		// 同异步模式
	unsigned char	readOnly:1;		// 只读模式打开
	unsigned char	createOnly:1;		// 供mem_tool使用
	unsigned char   emptyFilter:1;		// 是否启用空节点过滤功能
	unsigned char   autoDeleteDirtyShm:1;	// 是否需要在检出到内存不完整时自动删除并重建共享内存

	inline void Init(int keyFormat, unsigned long cacheSize, unsigned int createVersion)
	{
		// calculate buckettotal
		keySize = keyFormat;
		ipcMemSize = cacheSize;
		version = createVersion;
	}

} CacheInfo;

class CPurgeNodeNotifier {
	public:
		CPurgeNodeNotifier(){};
		virtual ~CPurgeNodeNotifier(){};
		virtual void PurgeNodeNotify(const char *key, CNode node) = 0;
};

class CCacheProcess;
class CRawDataProcess;
class CTreeDataProcess;
class CCachePool : private CTimerObject
{
	protected:
		CPurgeNodeNotifier  *_purge_notifier;
		CSharedMemory   _shm;		//共享内存管理器
		CacheInfo       _cacheInfo;	//cache基本信息

		CHash          *_hash;	        // hash桶
		CNGInfo        *_ngInfo;	// node管理
		CFeature       *_feature;	// 特性抽象
		CNodeIndex     *_nodeIndex;   	// NodeID转换
		//CTableInfo   *_tableInfo;	// Table信息

		char            _errmsg[256];
		int		_need_set_integrity;

		/* 待淘汰节点数目 */
		unsigned _need_purge_node_count;

		CTimerList * _delay_purge_timerlist;
		unsigned firstMarkerTime;
		unsigned lastMarkerTime;
		int emptyLimit;
        /**********for purge alert*******/
		int disableTryPurge;
        //如果自动淘汰的数据最后更新时间比当前时间减DataExpireAlertTime小则报警
		int dateExpireAlertTime;
        CRawData    *_raw_data;
        CRowValue _stRow;
		

	protected:
		/* for statistic*/
		CStatItemU32	statCacheSize;
		CStatItemU32	statCacheKey;
		CStatItemU32	statCacheVersion;
		CStatItemU32	statUpdateMode;
		CStatItemU32	statEmptyFilter;
		CStatItemU32	statHashSize;
		CStatItemU32	statFreeBucket;
		CStatItemU32	statDirtyEldest;
		CStatItemU32	statDirtyAge;
		CStatSample	statTryPurgeCount;
		CStatItemU32	statTryPurgeNodes;
		CStatItemU32	statLastPurgeNodeModTime;//最后被淘汰的节点的lastcmod的最大值(如果多行)
		CStatItemU32	statDataExistTime;//当前时间减去statLastPurgeNodeModTime
		CStatSample  survival_hour;  	
	private:
		int AppStorageOpen();
		int TTCMemOpen(APP_STORAGE_T *);
		int TTCMemAttach(APP_STORAGE_T *);
		int TTCMemInit(APP_STORAGE_T *);
		int VerifyCacheInfo(CacheInfo *);
		unsigned int HashBucketNum(uint64_t);

		int RemoveFromHashBase(const char *key, CNode node, int newhash);
		int RemoveFromHash(const char *key, CNode node);
		int MoveToNewHash(const char *key, CNode node);
		int Insert2Hash(const char *key, CNode node);

		int PurgeNode(const char *key, CNode purge_node);
		int PurgeNodeEverything(CNode purge_node);
		int PurgeNodeEverything(const char* key, CNode purge_node);
        
        /* purge alert*/
        int CheckAndPurgeNodeEverything(CNode purge_node);
        uint32_t GetCmodtime(CNode* purge_node);

		/* lru list op */
		int Insert2DirtyLru(CNode node) {return _ngInfo->Insert2DirtyLru(node);}
		int Insert2CleanLru(CNode node) {return _ngInfo->Insert2CleanLru(node);}
		int Insert2EmptyLru(CNode node) {
			return emptyLimit ?
				_ngInfo->Insert2EmptyLru(node) :
				_ngInfo->Insert2CleanLru(node) ;

		}
		int RemoveFromLru(CNode node)   {return _ngInfo->RemoveFromLru(node);}
		int KeyCmp(const char *key, const char *other);

		/* node|row count statistic for async flush.*/
		void IncDirtyNode(int v){ _ngInfo->IncDirtyNode(v);}
		void IncDirtyRow(int v) { _ngInfo->IncDirtyRow(v); }
		void IncTotalRow(int v) { _ngInfo->IncTotalRow(v); }
		void DecEmptyNode(void) { if(emptyLimit) _ngInfo->IncEmptyNode(-1); }
		void IncEmptyNode(void) {
			if(emptyLimit) {
				_ngInfo->IncEmptyNode(1);
				if(_ngInfo->EmptyCount() > emptyLimit) {
					PurgeSingleEmptyNode();
				}
			}
		}

		const unsigned int TotalDirtyNode() const {return _ngInfo->TotalDirtyNode();}
		const unsigned int TotalUsedNode() const {return _ngInfo->TotalUsedNode();}

		const uint64_t TotalDirtyRow() const {return _ngInfo->TotalDirtyRow();}
		const uint64_t TotalUsedRow() const {return _ngInfo->TotalUsedRow();}

		/*定期调度delay purge任务*/
		virtual void TimerNotify(void);

	public:
		CCachePool(CPurgeNodeNotifier *o = NULL);
		~CCachePool();

		int CacheOpen(CacheInfo *);
		void SetEmptyNodeLimit(int v) { emptyLimit = v<0?0:v; }
		int InitEmptyNodeList(void);
		int UpgradeEmptyNodeList(void);
		int MergeEmptyNodeList(void);
		int PruneEmptyNodeList(void);
		int ShrinkEmptyNodeList(void);
		int PurgeSingleEmptyNode(void);

		CNode CacheFind(const char *key, int newhash);
		CNode CacheFindAutoChoseHash(const char *key);
		int   CachePurge(const char *key);
		CNode CacheAllocate(const char *key);
		int   TryPurgeSize(size_t size, CNode purge_node, unsigned count=2500);
		void  DisableTryPurge(void) { disableTryPurge = 1; }
        void  SetDateExpireAlertTime(int time){dateExpireAlertTime = time<0?0:time;};

		/* 淘汰固定个节点 */
		void DelayPurgeNotify(const unsigned count=50);
		int PrePurgeNodes(int purge_cnt, CNode reserve);
		int PurgeByTime(unsigned int oldesttime);
		void StartDelayPurgeTask(CTimerList *);

		int   InsertTimeMarker(unsigned int);
		int   RemoveTimeMarker(CNode node);
		int   IsTimeMarker(CNode node) const;
		CNode FirstTimeMarker() const;
		CNode LastTimeMarker()  const;
		unsigned int FirstTimeMarkerTime();
		unsigned int LastTimeMarkerTime();

		CNode DirtyLruHead() const;
		CNode CleanLruHead() const;
		CNode EmptyLruHead() const;
		int   DirtyLruEmpty()const{return NODE_LIST_EMPTY(DirtyLruHead());}

		const CacheInfo* GetCacheInfo() const { return &_cacheInfo;}
		const char *Error(void) const { return _errmsg; }

		FEATURE_INFO_T* QueryFeatureById(const uint32_t id) 
		{
			return _feature ? _feature->GetFeatureById(id):(FEATURE_INFO_T *)(0);
		}

		int AddFeature(const uint32_t id, const MEM_HANDLE_T v)
		{
			if(_feature == NULL)
				return -1;
			return _feature->AddFeature(id, v);
		}

		uint32_t MaxNodeID(void) const{
			return _ngInfo->MaxNodeID();
		}

		NODE_ID_T MinValidNodeID(void) const {
			return  _ngInfo->MinValidNodeID();
		}

		static int32_t NodeRowsCount(CNode node) {
			if(!node || node.VDHandle() == INVALID_HANDLE)
				return 0;

			CDataChunk *chunk = ((CDataChunk*)(CBinMalloc::Instance()->Handle2Ptr(node.VDHandle())));
			if(!chunk) return 0;

			return chunk->TotalRows();
		}

		friend class CCacheProcess;
		friend class CRawDataProcess;
		friend class CTreeDataProcess;
};

TTC_END_NAMESPACE

#endif

