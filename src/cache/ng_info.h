/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords:  实现NodeGroup、Node的释放、分配，时间链表操作函数等
 * 
 *       This is unpublished proprietary
 *       source code of Tencent Ltd.
 *       The copyright notice above does not
 *       evidence any actual or intended
 *       publication of such source code.
 *
 *       NOTICE: UNAUTHORIZED DISTRIBUTION,ADAPTATION OR 
 *       USE MAY BE SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *  $Id: ng_info.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */
#ifndef __TTC_NG_INFO_H
#define __TTC_NG_INFO_H

#include <stdint.h>
#include "StatTTC.h"
#include "singleton.h"
#include "namespace.h"
#include "global.h"
#include "ng_list.h"

TTC_BEGIN_NAMESPACE


/* high-level 层支持的cache种类*/
enum MEM_CACHE_TYPE_T
{
    MEM_TTC_TYPE = 0x1UL,
    MEM_BMP_TYPE = 0x2UL,
};

/* high-level 层cache的签名、版本、类型等*/
#define MEM_CACHE_SIGN		0xFF00FF00FF00FF00ULL
#define MEM_CACHE_VERSION	0x1ULL
#define MEM_CACHE_TYPE		MEM_TTC_TYPE

struct cache_info
{
    uint64_t ci_sign;
    uint64_t ci_version;
    uint64_t ci_type;
};
typedef struct cache_info CACHE_INFO_T;

/* Low-Level预留了4k的空间，供后续扩展 */
/* TODO: 增加更加细致的逻辑判断*/
struct app_storage
{
    CACHE_INFO_T as_cache_info;
    MEM_HANDLE_T as_extend_info;

    int NeedFormat()
    {
        return  (as_cache_info.ci_sign != MEM_CACHE_SIGN) || 
            (INVALID_HANDLE == as_extend_info) ;
    }

    int Format(MEM_HANDLE_T v)
    {
        as_cache_info.ci_sign = MEM_CACHE_SIGN;
        as_cache_info.ci_version = MEM_CACHE_VERSION;
        as_cache_info.ci_type = MEM_TTC_TYPE;

        as_extend_info = v;
        return 0;
    }
};
typedef struct app_storage APP_STORAGE_T;

struct ng_info
{
    NG_LIST_T    ni_free_head; 		//有空闲Node的NG链表
    NG_LIST_T    ni_full_head;		//Node分配完的NG链表
    NODE_ID_T    ni_min_id;		//下一个被分配NG的起始NodeId
    MEM_HANDLE_T ni_sys_zone;   	//第一个NG为系统保留

    /*以下为统计值，用来控制异步flush的起停，速度等*/
    uint32_t    ni_used_ng;
    uint32_t    ni_used_node;
    uint32_t    ni_dirty_node;
    uint64_t    ni_used_row;
    uint64_t    ni_dirty_row;
};
typedef struct ng_info NG_INFO_T;

class CNGInfo
{
    public:
        CNGInfo();
        ~CNGInfo();

        static CNGInfo* Instance(){return CSingleton<CNGInfo>::Instance();}
        static void Destroy(){CSingleton<CNGInfo>::Destroy();}

        CNode  AllocateNode(void);		//分配一个新Node
        int    ReleaseNode(CNode&);		//归还CNode到所属的NG并摧毁自己

        /*statistic, for async flush */
        void IncDirtyNode(int v){ _ngInfo->ni_dirty_node += v; statDirtyNode = _ngInfo->ni_dirty_node; }
        void IncDirtyRow(int v) { _ngInfo->ni_dirty_row += v; statDirtyRow = _ngInfo->ni_dirty_row; }
        void IncTotalRow(int v) { _ngInfo->ni_used_row += v; statUsedRow = _ngInfo->ni_used_row; }
        void IncEmptyNode(int v) { emptyCnt += v; statEmptyNode = emptyCnt; }

        const unsigned int TotalDirtyNode() const { return _ngInfo->ni_dirty_node;}
        const unsigned int TotalUsedNode() const {return _ngInfo->ni_used_node; }

        const uint64_t TotalDirtyRow() const { return _ngInfo->ni_dirty_row;}
        const uint64_t TotalUsedRow() const { return _ngInfo->ni_used_row; }

        CNode  DirtyNodeHead();
        CNode  CleanNodeHead();
        CNode  EmptyNodeHead();


	/* 获取最小可用的NodeID */
	NODE_ID_T MinValidNodeID() const {return (NODE_ID_T)256;}

	/* 获取目前分配的最大NodeID */
	/* 由于目前node-group大小固定，而且分配后不会释放，因此可以直接通过已用的node-group算出来 */
	NODE_ID_T MaxNodeID() const { return _ngInfo->ni_used_ng*256-1; }

        //time-list op
        int Insert2DirtyLru(CNode);
        int Insert2CleanLru(CNode);
        int Insert2EmptyLru(CNode);
        int RemoveFromLru(CNode);
	int EmptyCount(void) const { return emptyCnt; }
	enum {
		CREATED,	// this memory is fresh
		ATTACHED,	// this is an old memory, and empty lru present
		UPGRADED	// this is an old memory, and empty lru is missing
	};
	int EmptyStartupMode(void) const { return emptyStartupMode; }

        const MEM_HANDLE_T Handle() const {return M_HANDLE(_ngInfo);}
        const char *Error() const {return _errmsg;}

        //创建物理内存并格式化
        int Init(void);
        //绑定到物理内存
        int Attach(MEM_HANDLE_T handle);
        //脱离物理内存
        int Detach(void);

    protected:

        int InitHeader(NG_INFO_T *);

        NODE_GROUP_T * AllocateNG(void);
        NODE_GROUP_T * FindFreeNG(void);

        void ListDel(NODE_GROUP_T *);
        void FreeListAdd(NODE_GROUP_T *);
        void FullListAdd(NODE_GROUP_T *);
        void FullListAddTail(NODE_GROUP_T *);
        void FreeListAddTail(NODE_GROUP_T *);

    private:
        NG_INFO_T *_ngInfo;
        char       _errmsg[256];
	// the total empty node present
	int        emptyCnt;
	int	   emptyStartupMode;

    private:
        CStatItemU32 statUsedNG;
        CStatItemU32 statUsedNode;
        CStatItemU32 statDirtyNode;
        CStatItemU32 statEmptyNode;
        CStatItemU32 statUsedRow;
        CStatItemU32 statDirtyRow;
};

TTC_END_NAMESPACE

#endif

/* ends here */
