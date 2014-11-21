#include <string.h>
#include <stdio.h>
#include "node_group.h"
#include "node_list.h"
#include "node_index.h"
#include "ng_info.h"
#include "node.h"
#include "ttc_global.h"

TTC_USING_NAMESPACE

CNGInfo::CNGInfo(): _ngInfo(NULL)
{
    memset(_errmsg, 0, sizeof(_errmsg));
    emptyCnt = 0;
    emptyStartupMode = CREATED;

    statUsedNG 	= statmgr.GetItemU32(TTC_USED_NGS);
    statUsedNode 	= statmgr.GetItemU32(TTC_USED_NODES);
    statDirtyNode	= statmgr.GetItemU32(TTC_DIRTY_NODES);
    statEmptyNode	= statmgr.GetItemU32(TTC_EMPTY_NODES);
    statEmptyNode	= 0;
    statUsedRow	= statmgr.GetItemU32(TTC_USED_ROWS);
    statDirtyRow	= statmgr.GetItemU32(TTC_DIRTY_ROWS);
}

CNGInfo::~CNGInfo()
{
}

CNode CNGInfo::AllocateNode(void)
{
    //优先在空闲链表分配
    NODE_GROUP_T *NG = FindFreeNG();
    if(!NG){
	    /* 防止NodeGroup把内存碎片化，采用预分配 */
	    static int step = CTTCGlobal::_pre_alloc_NG_num;
	    static int fail = 0;
	    for(int i=0; i<step; i++) {
		    NG = AllocateNG();
		    if(!NG){
			    if(i == 0)
				    return CNode();
			    else
			    {
				    fail = 1;
				    step = 1;
				    break;
			    }
		    }

		    FreeListAdd(NG);
	    }

	    /* find again */
	    NG = FindFreeNG();

	    if(step<256 && !fail)
		    step *= 2;
    }

    CNode node = NG->AllocateNode();
    //NG中没有任何可分配的Node
    if(NG->IsFull()){
        ListDel(NG);
        FullListAdd(NG);
    }

    if(!node)
    {
        snprintf(_errmsg, sizeof(_errmsg), "PANIC: allocate node failed");
        return CNode();
    }

    //statistic
    _ngInfo->ni_used_node ++;
    statUsedNode = _ngInfo->ni_used_node;

    //insert to node_index
    I_INSERT(node);
    return node;
}

int CNGInfo::ReleaseNode(CNode& node)
{
    NODE_GROUP_T *NG = node.Owner();
    if(NG->IsFull())
    {
        //NG挂入空闲链表
        ListDel(NG);
        FreeListAdd(NG);
    }

    _ngInfo->ni_used_node --;
    statUsedNode = _ngInfo->ni_used_node;
    return node.Release();
}

CNode CNGInfo::DirtyNodeHead()
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    if(!sysNG)
        return CNode();
    return CNode(sysNG, SYS_DIRTY_NODE_INDEX);
}

CNode CNGInfo::CleanNodeHead()
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    if(!sysNG)
        return CNode();
    return CNode(sysNG, SYS_CLEAN_NODE_INDEX);
}

CNode CNGInfo::EmptyNodeHead()
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    if(!sysNG)
        return CNode();
    return CNode(sysNG, SYS_EMPTY_NODE_INDEX);
}

int CNGInfo::Insert2DirtyLru(CNode node)
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    CNode dirtyNode(sysNG, SYS_DIRTY_NODE_INDEX);

    NODE_LIST_ADD(node, dirtyNode);

    return 0;
}

int CNGInfo::Insert2CleanLru(CNode node)
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    CNode cleanNode(sysNG, SYS_CLEAN_NODE_INDEX);

    NODE_LIST_ADD(node, cleanNode);

    return 0;
}

int CNGInfo::Insert2EmptyLru(CNode node)
{
    NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
    CNode emptyNode(sysNG, SYS_EMPTY_NODE_INDEX);

    NODE_LIST_ADD(node, emptyNode);

    return 0;
}

int CNGInfo::RemoveFromLru(CNode node)
{
    NODE_LIST_DEL(node);
    return 0;
}

NODE_GROUP_T* CNGInfo::AllocateNG(void)
{
    MEM_HANDLE_T v = M_CALLOC(NODE_GROUP_T::Size());
    if(INVALID_HANDLE == v)
    {
        snprintf(_errmsg, sizeof(_errmsg), "allocate nodegroup failed, %s", M_ERROR());
        return (NODE_GROUP_T *)0;
    }

    NODE_GROUP_T *NG = M_POINTER(NODE_GROUP_T, v);
    NG->Init(_ngInfo->ni_min_id);
    _ngInfo->ni_min_id += NODE_GROUP_INCLUDE_NODES;
    _ngInfo->ni_used_ng ++;
    statUsedNG = _ngInfo->ni_used_ng;

    return NG;
}


NODE_GROUP_T* CNGInfo::FindFreeNG(void)
{
    //链表为空
    if(NG_LIST_EMPTY(&(_ngInfo->ni_free_head)))
    {
        return (NODE_GROUP_T *) 0;
    }

    return NG_LIST_ENTRY(_ngInfo->ni_free_head.Next(), NODE_GROUP_T, ng_list);
}


void CNGInfo::ListDel(NODE_GROUP_T *NG)
{
    NG_LIST_T *p = &(NG->ng_list);
    return NG_LIST_DEL(p);
}


#define EXPORT_NG_LIST_FUNCTION(name, member, function)		\
    void CNGInfo::name(NODE_GROUP_T *NG)	     		\
{								\
    NG_LIST_T *p    = &(NG->ng_list);			\
    NG_LIST_T *head = &(_ngInfo->member);			\
    return function(p, head);				\
}


EXPORT_NG_LIST_FUNCTION(FreeListAdd, 	 ni_free_head, NG_LIST_ADD)
EXPORT_NG_LIST_FUNCTION(FullListAdd, 	 ni_full_head, NG_LIST_ADD)
EXPORT_NG_LIST_FUNCTION(FreeListAddTail, ni_free_head, NG_LIST_ADD_TAIL)
EXPORT_NG_LIST_FUNCTION(FullListAddTail, ni_full_head, NG_LIST_ADD_TAIL)



int CNGInfo::InitHeader(NG_INFO_T *ni)
{
    INIT_NG_LIST_HEAD(&(ni->ni_free_head));
    INIT_NG_LIST_HEAD(&(ni->ni_full_head));

    ni->ni_min_id = SYS_MIN_NODE_ID;

    /* init system reserved zone*/
    { 
        NODE_GROUP_T *sysNG = AllocateNG();
        if(!sysNG)
            return -1;

        sysNG->SystemReservedInit();
        ni->ni_sys_zone = M_HANDLE(sysNG);
    }

    ni->ni_used_ng    = 1;
    ni->ni_used_node  = 0;
    ni->ni_dirty_node = 0;
    ni->ni_used_row   = 0;
    ni->ni_dirty_row  = 0;

    statUsedNG 	      = ni->ni_used_ng;
    statUsedNode      = ni->ni_used_node;
    statDirtyNode     = ni->ni_dirty_node;
    statDirtyRow      = ni->ni_dirty_row;
    statUsedRow       = ni->ni_used_row;
    statEmptyNode     = 0;

    return 0;
}


int CNGInfo::Init(void)
{
    //1. malloc ng_info mem.
    MEM_HANDLE_T v = M_CALLOC(sizeof(NG_INFO_T));
    if(INVALID_HANDLE == v)
    {
        snprintf(_errmsg, sizeof(_errmsg), "init nginfo failed, %s", M_ERROR());
        return -1;
    }

    //2. mapping
    _ngInfo = M_POINTER(NG_INFO_T, v);

    //3. init header
    return InitHeader(_ngInfo);
}

int CNGInfo::Attach(MEM_HANDLE_T v)
{
    if(INVALID_HANDLE == v)
    {
        snprintf(_errmsg, sizeof(_errmsg), "attach nginfo failed, memory handle = 0");
        return -1;
    }

    _ngInfo = M_POINTER(NG_INFO_T, v);

    /* check system reserved zone:
     *   1. the present of empty lru list
     */
    { 
        NODE_GROUP_T *sysNG = M_POINTER(NODE_GROUP_T, _ngInfo->ni_sys_zone);
        if(!sysNG)
            return -1;

        int ret = sysNG->SystemReservedCheck();
	if(ret < 0)
		return ret;
	if(ret > 0) {
		emptyStartupMode = UPGRADED;
	} else {
		emptyStartupMode = ATTACHED;
	}
    }

    return 0;
}

int CNGInfo::Detach(void)
{
    _ngInfo = NULL;
    return 0;
}

