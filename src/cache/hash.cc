#include <string.h>
#include <stdio.h>
#include "hash.h"
#include "global.h"

TTC_USING_NAMESPACE

CHash::CHash() :
    _hash(NULL)
{
    memset(_errmsg, 0, sizeof(_errmsg));
}

CHash::~CHash()
{
}

NODE_ID_T& CHash::Hash2Node(const HASH_ID_T v)
{
    return _hash->hh_buckets[v];
}

int CHash::Init(const uint32_t hsize, const uint32_t fixedsize)
{
    size_t size = sizeof(NODE_ID_T);
    size *= hsize;
    size += sizeof(HASH_T);

    MEM_HANDLE_T v = M_CALLOC(size);
    if(INVALID_HANDLE == v)
    {
        snprintf(_errmsg, sizeof(_errmsg), "init hash bucket failed, %s", M_ERROR());
	return -1;
    }

    _hash = M_POINTER(HASH_T, v);
    _hash->hh_size = hsize;
    _hash->hh_free = hsize;
    _hash->hh_node = 0;
    _hash->hh_fixedsize = fixedsize;

    /* init each nodeid to invalid */
    for(uint32_t i = 0; i < hsize; i++)
    {
        _hash->hh_buckets[i] = INVALID_NODE_ID;
    }
    
    return 0;
}

int CHash::Attach(MEM_HANDLE_T handle)
{
    if(INVALID_HANDLE == handle)
    {
        snprintf(_errmsg, sizeof(_errmsg), "attach hash bucket failed, memory handle = 0");
	return -1;
    }

    _hash = M_POINTER(HASH_T, handle);
    return 0;
}

int CHash::Detach(void)
{
    _hash = (HASH_T *)(0);
    return 0;
}
