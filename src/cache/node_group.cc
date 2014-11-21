#include "node_group.h"
#include "node_index.h"
#include "node_list.h"
#include "global.h"
#include "node.h"

TTC_USING_NAMESPACE

//定义每种属性的内存大小, 至少有以下四种，可以再增加
const uint32_t NODE_GROUP_T::NG_ATTR_SIZE[] =
{
    NODE_GROUP_INCLUDE_NODES * sizeof(NODE_ID_T),        //NEXT_NODE
    NODE_GROUP_INCLUDE_NODES * sizeof(NODE_ID_T)*2,      //TIME_LIST
    NODE_GROUP_INCLUDE_NODES * sizeof(MEM_HANDLE_T),     //VD_HANDLE
    NODE_GROUP_INCLUDE_NODES / 8,                        //DIRTY_BMP 
};

int NODE_GROUP_T::Init(NODE_ID_T id)
{
    ng_list.prev  = ng_list.next = INVALID_HANDLE;
    ng_dele.top   = 0;
    ng_dele.count = 0;
    ng_free       = 0;
    ng_nid	  = id;

    //属性
    ng_attr.count = AttrCount();
    ng_attr.offset[0] = BaseHeaderSize();
    for(unsigned int i =1; i < ng_attr.count; i++)
    {
        ng_attr.offset[i] = ng_attr.offset[i-1] + NG_ATTR_SIZE[i-1];
    }

    /* 初始化每个Node */
    for(unsigned i=0; i < NODE_GROUP_INCLUDE_NODES; ++i)
    {
	    NextNodeID(i)  = INVALID_NODE_ID;
	    NODE_ID_T *lru = NodeLru(i);
	    lru[LRU_PREV]  = NodeID(i);
	    lru[LRU_NEXT]  = NodeID(i);
	    VDHandle(i)    = INVALID_HANDLE;
	    ClrDirty(i);
    }

    return 0;
}

/* init system reserved zone */
int NODE_GROUP_T::SystemReservedInit()
{
    CNode dirtyNode = AllocateNode();
    if(!dirtyNode){
        return -2;
    }

    CNode cleanNode = AllocateNode();
    if(!cleanNode){
        return -3;
    }

    CNode emptyNode = AllocateNode();
    if(!emptyNode){
        return -3;
    }

    /* init node list head */
    INIT_NODE_LIST_HEAD(dirtyNode, dirtyNode.NodeID());
    INIT_NODE_LIST_HEAD(cleanNode, cleanNode.NodeID());
    INIT_NODE_LIST_HEAD(emptyNode, emptyNode.NodeID());

    /* insert node head's node-id to node-index*/
    I_INSERT(dirtyNode);
    I_INSERT(cleanNode);
    I_INSERT(emptyNode);

    return 0;
}

/* check system reserved zone integrity
 * the main purpose is upgrade/add the missing empty lru list 
 */
int NODE_GROUP_T::SystemReservedCheck()
{
    // ng_free<2 is invalid, no dirty lru and clean lru???
    if(ng_free < 2)
	    return -10;
    // ng_free==2 old format, index 2 is free & reserved
    // ng_free==3 new format, index 2 allocated to emptyNodeLru
    int hasEmptyLru1 = ng_free>=3;

    // if new format, index 2 is allocated, lru pointer should be non-zero
    /* bug: 因为lru已经初始化，不能用0来判断
    NODE_ID_T *lru = NodeLru(SYS_EMPTY_NODE_INDEX);
    int hasEmptyLru2 = lru[LRU_PREV] != 0;
    int hasEmptyLru3 = lru[LRU_NEXT] != 0;

    if(hasEmptyLru1 != hasEmptyLru2 || hasEmptyLru2 != hasEmptyLru3)
	    return -11;
    */

    // sanity check passed
    if(hasEmptyLru1 == 0) {
	    // no empty lru, allocate one
	    CNode emptyNode = AllocateNode();
	    if(!emptyNode){
		    return -3;
	    }

	    /* init node list head */
	    INIT_NODE_LIST_HEAD(emptyNode, emptyNode.NodeID());

	    /* insert node head's node-id to node-index*/
	    I_INSERT(emptyNode);
	    return 1;
    }

    return 0;
}

CNode NODE_GROUP_T::AllocateNode(void)
{
    if(IsFull())
    {
        return CNode(NULL, 0);
    }

    //优先分配release掉的Node空间
    if(ng_dele.count > 0)
    {
        CNode N(this, ng_dele.top);
        N.Reset();

        ng_dele.count--;
        ng_dele.top = (uint8_t)N.VDHandle();

        return N;
    }
    //在空闲Node中分配
    else
    {
        CNode N(this, ng_free);
        N.Reset();

        ng_free++;
        return N;
    }
}

int NODE_GROUP_T::ReleaseNode(CNode N)
{
    //复用node的handle attribute空间来把释放掉的node组织为单链表
    N.VDHandle() = ng_dele.top;
    ng_dele.top = N.Index();
    ng_dele.count++;

    return 0;
}

bool NODE_GROUP_T::IsFull(void)
{
    return (ng_dele.count == 0 && ng_free >= NODE_GROUP_INCLUDE_NODES);
}

uint32_t NODE_GROUP_T::AttrCount(void)
{
    return sizeof(NG_ATTR_SIZE)/sizeof(uint32_t);
}

uint32_t NODE_GROUP_T::BaseHeaderSize(void)
{
    return OFFSETOF(NODE_GROUP_T, ng_attr) + OFFSETOF(NG_ATTR_T, offset) + sizeof(uint32_t)*AttrCount();
}

uint32_t NODE_GROUP_T::AttrSize(void)
{
    uint32_t size = 0;

    for(uint32_t i = 0; i < AttrCount(); i++)
    {
        size += NG_ATTR_SIZE[i];
    }

    return size;
}

uint32_t NODE_GROUP_T::Size(void)
{
    return BaseHeaderSize() + AttrSize();
}

NODE_ID_T NODE_GROUP_T::NodeID(int idx) const 
{
    return (ng_nid + idx);
}

NODE_ID_T & NODE_GROUP_T::NextNodeID(int idx)
{
    return  __CAST__<NODE_ID_T>(NEXT_NODE)[idx];
}

NODE_ID_T * NODE_GROUP_T::NodeLru(int idx)
{
    return &(__CAST__<NODE_ID_T>(TIME_LIST)[idx*2]);
}

MEM_HANDLE_T & NODE_GROUP_T::VDHandle(int idx)
{
    return __CAST__<MEM_HANDLE_T>(VD_HANDLE)[idx];
}

bool NODE_GROUP_T::IsDirty(int idx)
{
    return FD_ISSET(idx, __CAST__<fd_set>(DIRTY_BMP));
}

void NODE_GROUP_T::SetDirty(int idx)
{
    FD_SET(idx, __CAST__<fd_set>(DIRTY_BMP));
}

void NODE_GROUP_T::ClrDirty(int idx)
{
    FD_CLR(idx, __CAST__<fd_set>(DIRTY_BMP));
}
