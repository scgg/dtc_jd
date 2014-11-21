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

//nodegroup�ͷŵ���node����
struct ng_delete
{
    uint16_t  top;
    uint16_t count;
};
typedef struct ng_delete NG_DELE_T;

//nodegroup����
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
	uint8_t   ng_rsv[2];	//�����ռ�
	NODE_ID_T ng_nid;
	NG_ATTR_T ng_attr;

    private:

	CNode  AllocateNode(void);		  	// ����һ��Node
	int    ReleaseNode(CNode);			// �ͷ�һ��Node
	bool   IsFull(void);		 		// NodeGroup�Ƿ��Ѿ�������
	int    Init(NODE_ID_T id);		  	// NodeGroup��ʼ��
	int    SystemReservedInit();		 	// ϵͳ������NG��ʼ��
	// this routine return:
	//    0,  passed, empty lru present
	//    1,  passed, empty lru created
	//    <0, integrity error
	int    SystemReservedCheck();		 	// ϵͳ������NGһ���Լ��
	static uint32_t Size(void);	  		// ����nodegroup���ܴ�С

    private:

	//���Բ����ӿڣ���CNode����
	NODE_ID_T     NodeID(int idx) const;
	NODE_ID_T&    NextNodeID(int idx);		// attr1]   -> ��һ��Node��NodeID
	NODE_ID_T*    NodeLru(int idx);			// attr[2]   -> LRU����
	MEM_HANDLE_T& VDHandle(int idx);		// attr[3]   -> ����handle
	bool IsDirty(int idx);				// attr[4]   -> ��λͼ
	void SetDirty(int idx);
	void ClrDirty(int idx);

	//����ÿ�����Կ����ʼ��ַ
	template <class T>
	    T * __CAST__(ATTR_TYPE_T t) { return (T *)((char *)this + ng_attr.offset[t]);}

    private:
	static uint32_t AttrCount(void);		// ֧�ֵ����Ը���
	static uint32_t AttrSize(void);			// �������Ե��ڴ��С 
	static uint32_t BaseHeaderSize(void);		// ���������⣬Nodegroup�Ĵ�С
	static const uint32_t NG_ATTR_SIZE[];

	friend class CNode;
	friend class CNGInfo;
};
typedef struct node_group NODE_GROUP_T;


TTC_END_NAMESPACE

#endif
