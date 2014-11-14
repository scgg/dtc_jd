/*
 * =====================================================================================
 *
 *       Filename:  check_cache_holder.h
 *
 *    Description:  share memory manager for checker
 *
 *        Version:  1.0
 *        Created:  03/20/2009 10:58:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_SHARE_MEMORY_H__
#define __TTC_CHECK_SHARE_MEMORY_H__

#include "noncopyable.h"
#include "shmem.h"
#include "global.h"
#include "node_list.h"
#include "node_index.h"
#include "node_group.h"
#include "feature.h"
#include "ng_info.h"
#include "hash.h"
#include "node.h"

TTC_BEGIN_NAMESPACE

class CCacheHolder: private noncopyable
{
public:
    CCacheHolder(void);
    ~CCacheHolder (void);

    static CCacheHolder* Instance (void);
    static void Destroy (void);

    int open (int shm_key);
    int close (void);
    
    CHash*      get_hash (void)
    {
        return _hash;
    }

    CNGInfo*    get_nginfo (void)
    {
        return _ngInfo;
    }

    CFeature*   get_feature (void)
    {
        return _feature;
    }

    CNodeIndex* get_node_index (void)
    {
        return _nodeIndex;
    }

public:

protected:
protected:

private:
    int storage_open (void);

private:
    CSharedMemory   _shm;
    CHash*          _hash;      // hash桶
    CNGInfo*        _ngInfo;    // node管理
    CFeature*       _feature;   // 特性抽象
    CNodeIndex*     _nodeIndex; // NodeID转换
};

TTC_END_NAMESPACE

#endif
