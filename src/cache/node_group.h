#ifndef __TTC_NODE_GROUP_H
#define __TTC_NODE_GROUP_H

#include <stdint.h>
#include "namespace.h"
#include "global.h"
#include "ng_list.h"

TTC_BEGIN_NAMESPACE

enum attr_type
{
    NEXT_NODE = 0,
    TIME_LIST = 1,
    VD_HANDLE = 2,
    DIRTY_BMP = 3,
};
typedef enum attr_type ATTR_TYPE_T;

//nodegroup释放掉的node链表
struct ng_delete
{
    uint16_t  top;
    uint16_t count;
};
typedef struct ng_delete NG_DELE_T;

//nodegroup属性
struct ng_attr
{
    uint32_t  count;
    uint32_t  offset[0]; 
};
typedef struct ng_attr NG_ATTR_T;

class CNode;
struct node_group
{
    public:
	NG_LIST_T ng_list;
	NG_DELE_T ng_dele;
	uint16_t  ng_free;
	uint8_t   ng_rsv[2];	//保留空间
	NODE_ID_T ng_nid;
	NG_ATTR_T ng_attr;

    private:

	CNode  AllocateNode(void);		  	// 分配一个Node
	int    ReleaseNode(CNode);			// 释放一个Node
	bool   IsFull(void);		 		// NodeGroup是否已经分配完
	int    Init(NODE_ID_T id);		  	// NodeGroup初始化
	int    SystemReservedInit();		 	// 系统保留的NG初始化
	// this routine return:
	//    0,  passed, empty lru present
	//    1,  passed, empty lru created
	//    <0, integrity error
	int    SystemReservedCheck();		 	// 系统保留的NG一致性检查
	static uint32_t Size(void);	  		// 返回nodegroup的总大小

    private:

	//属性操作接口，供CNode访问
	NODE_ID_T     NodeID(int idx) const;
	NODE_ID_T&    NextNodeID(int idx);		// attr1]   -> 下一个Node的NodeID
	NODE_ID_T*    NodeLru(int idx);			// attr[2]   -> LRU链表
	MEM_HANDLE_T& VDHandle(int idx);		// attr[3]   -> 数据handle
	bool IsDirty(int idx);				// attr[4]   -> 脏位图
	void SetDirty(int idx);
	void ClrDirty(int idx);

	//返回每种属性块的起始地址
	template <class T>
	    T * __CAST__(ATTR_TYPE_T t) { return (T *)((char *)this + ng_attr.offset[t]);}

    private:
	static uint32_t AttrCount(void);		// 支持的属性个数
	static uint32_t AttrSize(void);			// 所有属性的内存大小 
	static uint32_t BaseHeaderSize(void);		// 除开属性外，Nodegroup的大小
	static const uint32_t NG_ATTR_SIZE[];

	friend class CNode;
	friend class CNGInfo;
};
typedef struct node_group NODE_GROUP_T;


TTC_END_NAMESPACE

#endif
