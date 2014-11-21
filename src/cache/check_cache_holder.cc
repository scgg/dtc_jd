/*
 * =====================================================================================
 *
 *       Filename:  check_cache_holder.cc
 *
 *    Description:  share memory manager impl
 *
 *        Version:  1.0
 *        Created:  03/22/2009 03:48:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <errno.h>
#include <string.h>

#include "check_cache_holder.h"
#include "check_code.h"
#include "memcheck.h"

#define CACHE_READ_ONLY     1
#define CACHE_READ_WRITE    0
#define CACHE_SUPPORT_VER   4

TTC_BEGIN_NAMESPACE

CCacheHolder::CCacheHolder (void) : _hash(NULL), _ngInfo(NULL), _feature(NULL), _nodeIndex(NULL)
{

}

CCacheHolder::~CCacheHolder (void)
{

}

CCacheHolder* CCacheHolder::Instance (void)
{
    return CSingleton<CCacheHolder>::Instance ();
}

void CCacheHolder::Destroy (void)
{
    return CSingleton<CCacheHolder>::Destroy ();
}

int CCacheHolder::storage_open (void)
{
    APP_STORAGE_T* storage = M_POINTER (APP_STORAGE_T, CBinMalloc::Instance()->GetReserveZone());

    if(NULL == storage)
    {
        log_error ("get reserve zone from binmalloc failed: %s", M_ERROR());
        return CHECK_ERROR_HOLDER;
    }

    _feature = CFeature::Instance();
    if(!_feature || _feature->Attach(storage->as_extend_info))
    {
        log_error ("%s", _feature->Error());
        return CHECK_ERROR_HOLDER;
    }

    /*hash-bucket*/
    FEATURE_INFO_T* p = _feature->GetFeatureById(HASH_BUCKET);
    if(NULL == p)
    {
        log_error ("%s", "not found hash-bucket feature");
        return -1;
    }

    _hash = CHash::Instance();
    if(!_hash || _hash->Attach(p->fi_handle))
    {
        log_error ("%s", _hash->Error());
        return -1;
    }

    /*node-index*/
    p = _feature->GetFeatureById(NODE_INDEX);
    if(NULL == p)
    {
        log_error ("%s", "not found node-index feature");
        return CHECK_ERROR_HOLDER;
    }

    _nodeIndex = CNodeIndex::Instance();
    if(NULL == _nodeIndex || _nodeIndex->Attach(p->fi_handle))
    {
        log_error ("%s", _nodeIndex->Error());
        return CHECK_ERROR_HOLDER;
    }

    /*ng-info*/
    p = _feature->GetFeatureById(NODE_GROUP);
    if(NULL == p)
    {
        log_error ("%s", "not found ng-info feature");
        return -1;
    }

    _ngInfo = CNGInfo::Instance();  
    if(NULL == _ngInfo || _ngInfo->Attach(p->fi_handle))
    {
        log_error ("%s", _ngInfo->Error());
        return CHECK_ERROR_HOLDER;
    }

    return CHECK_SUCC;
}

int CCacheHolder::open (int shm_key)
{
    if (shm_key < 0)
    {
        log_error ("shm key[%d] is invalid", shm_key);
        return CHECK_ERROR_HOLDER;
    }

    /* open share memory */
    if (_shm.Open(shm_key) <= 0)
    {
        log_error ("shm key[%d] no exist", shm_key);
        return CHECK_ERROR_HOLDER;
    }

    /* lock share memory */
    if (_shm.Lock() != 0)
    {
        log_error ("lock the shm key[%d] failed, msg[%s]", shm_key, strerror(errno));
        return CHECK_ERROR_HOLDER;
    }

    /* attach share memory */
    if (_shm.Attach(CACHE_READ_WRITE) == NULL)
    {
        log_error ("attach shm[%d] failed, msg[%s]", shm_key, strerror(errno));
        return CHECK_ERROR_HOLDER;
    }

    /* attch bin malloc*/
    if (CBinMalloc::Instance()->Attach(_shm.Ptr(), _shm.Size()) != 0)
    {
        log_error ("bin malloc attach failed, msg[%s]", M_ERROR());
        return CHECK_ERROR_HOLDER;
    }

    int version = CBinMalloc::Instance()->DetectVersion ();
    if (version != CACHE_SUPPORT_VER)
    {
        log_error ("unsupport cache version[%d]", version);
        return CHECK_ERROR_HOLDER;
    }

    return storage_open ();
}

int CCacheHolder::close (void)
{
    DELETE(_hash);
    DELETE(_ngInfo);
    DELETE(_feature);
    DELETE(_nodeIndex);

    return 0;
}

TTC_END_NAMESPACE

