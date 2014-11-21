#include <string.h>
#include <stdio.h>
#include "node_index.h"
#include "singleton.h"
#include "node.h"

TTC_USING_NAMESPACE

CNodeIndex::CNodeIndex():
	_firstIndex(NULL)
{
	memset(_errmsg, 0, sizeof(_errmsg));
}

CNodeIndex::~CNodeIndex()
{
}

CNodeIndex* CNodeIndex::Instance()
{
	return CSingleton<CNodeIndex>::Instance();
}

void CNodeIndex::Destroy()
{
	CSingleton<CNodeIndex>::Destroy();
}

int CNodeIndex::PreAllocateIndex2(size_t mem_size) 
{
	/* 
	 * 按所有节点全为空节点来分配2级NodeIndex
	 * 一个空节点占用44 bytes
	 */
	uint32_t n = 65536*256*44;
	n = mem_size/n + 1 ;
	n = n > 256 ? 256:n; 

	for(uint32_t i = 0; i < n; ++i) 
	{
		_firstIndex->fi_h[i] = M_CALLOC(INDEX_2_SIZE);

		if(INVALID_HANDLE == _firstIndex->fi_h[i])
		{
			log_crit("PANIC: PrepareNodeIndex[%u] failed", i);
			return -1;
		}
	}

	return 0;
}

int CNodeIndex::Insert(CNode node)
{
	NODE_ID_T id = node.NodeID();

	if(INVALID_HANDLE == _firstIndex->fi_h[OFFSET1(id)])
	{
		_firstIndex->fi_h[OFFSET1(id)] = M_CALLOC(INDEX_2_SIZE);
		if(INVALID_HANDLE == _firstIndex->fi_h[OFFSET1(id)])
		{
			log_crit("PANIC: Insert node=%u to NodeIndex failed", id);
			return -1;
		}
	}

	SECOND_INDEX_T *p = M_POINTER(SECOND_INDEX_T, _firstIndex->fi_h[OFFSET1(id)]);
	p->si_used++;
	p->si_h[OFFSET2(id)] = M_HANDLE(node.Owner());

	return 0;
}

CNode CNodeIndex::Search(NODE_ID_T id)
{
	if(INVALID_NODE_ID == id)
		return CNode(NULL, 0);

	if(INVALID_HANDLE == _firstIndex->fi_h[OFFSET1(id)])
		return CNode(NULL, 0);

	SECOND_INDEX_T *p = M_POINTER(SECOND_INDEX_T, _firstIndex->fi_h[OFFSET1(id)]);
	if(INVALID_HANDLE == p->si_h[OFFSET2(id)])
		return CNode(NULL, 0);

	NODE_GROUP_T *NG = M_POINTER(NODE_GROUP_T, p->si_h[OFFSET2(id)]);

	int index = (id - NG->ng_nid);
	if(index < 0 || index > 255)
		return CNode(NULL, 0);

	return CNode(NG, index);
}

int CNodeIndex::Init(size_t mem_size)
{
	MEM_HANDLE_T v = M_CALLOC(INDEX_1_SIZE);
	if(INVALID_HANDLE == v)
	{
		log_crit("Create Index-1 failed");
		return -1;
	}

	_firstIndex = M_POINTER(FIRST_INDEX_T, v);

	return PreAllocateIndex2(mem_size);
}

int CNodeIndex::Attach(MEM_HANDLE_T handle)
{
	if(INVALID_HANDLE == handle)
	{
		log_crit("attach index-1 failed, memory handle=0");
		return -1;
	}

	_firstIndex = M_POINTER(FIRST_INDEX_T, handle);
	return 0;
}

int CNodeIndex::Detach(void)
{
	_firstIndex = 0;
	return 0;
}
