/*
 * =====================================================================================
 *
 *       Filename:  check_result.h
 *
 *    Description:  存放hash, lru检测后的结果值
 *
 *        Version:  1.0
 *        Created:  03/18/2009 11:59:32 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_RESULT_H__
#define __TTC_CHECK_RESULT_H__
#include <vector>

#include "noncopyable.h"
#include "global.h"
#include "check_result_info.h"

TTC_BEGIN_NAMESPACE

class CCheckResult : private noncopyable
{
public:
    typedef std::vector<lru_node_info_t> LRU_NODE_INFO_T;
    typedef std::vector<ERR_NODE_INFO> NODE_CONTAINER_T;

public:
    CCheckResult (void);
    ~CCheckResult (void);

    static CCheckResult* Instance (void);
    static void Destroy (void);

    int open (void);
    int close (void);

    int save_dirty_lru_node_info (lru_node_info_t & node_info);
    int save_dirty_lru_node_info_reverse (lru_node_info_t & node_info);
    int save_clean_lru_node_info (lru_node_info_t & node_info);
    int save_clean_lru_node_info_reverse (lru_node_info_t & node_info);
    int save_empty_lru_node_info (lru_node_info_t & node_info);
    int save_empty_lru_node_info_reverse (lru_node_info_t & node_info);

    LRU_NODE_INFO_T & get_dirty_lru_node_info()
    {
        return _dirty_lru_node_info;
    }

    LRU_NODE_INFO_T & get_dirty_lru_node_info_reverse()
    {
        return _dirty_lru_node_info_reverse;
    }

    LRU_NODE_INFO_T & get_clean_lru_node_info()
    {
        return _clean_lru_node_info;
    }

    LRU_NODE_INFO_T & get_clean_lru_node_info_reverse()
    {
        return _clean_lru_node_info_reverse;
    }

    LRU_NODE_INFO_T & get_empty_lru_node_info()
    {
        return _empty_lru_node_info;
    }

    LRU_NODE_INFO_T & get_empty_lru_node_info_reverse()
    {
        return _empty_lru_node_info_reverse;
    }

    int save_hash_handle (const NODE_ID_T node_id, const MEM_HANDLE_T mem_handle);
    int save_lru_handle (const NODE_ID_T node_id, const MEM_HANDLE_T mem_handle);

    NODE_CONTAINER_T& get_hash_nodes (void)
    {
        return _hash_nodes;
    }


    NODE_CONTAINER_T& get_lru_nodes (void)
    {
        return _lru_nodes;
    }

    //how many node checked on clean node head list, forward check
    void set_next_count (const uint64_t count)
    {
        _next_count = count;
    }

    //how many node checked on clean node head list, back check
    void set_prev_count (const uint64_t count)
    {
        _prev_count = count;
    }

    uint64_t get_next_count (void)
    {
        return _next_count;
    }

    uint64_t get_prev_count (void)
    {
        return _prev_count;
    }

    int64_t node_count_diff (void);

public:

protected:
protected:

private:
private:
    //how many node checked on clean node head list, forward check
    uint64_t            _next_count;
    //how many node checked on clean node head list, back check
    uint64_t            _prev_count;
   
    //vector of ill nodes in hash
    LRU_NODE_INFO_T     _dirty_lru_node_info;
    LRU_NODE_INFO_T     _dirty_lru_node_info_reverse;
    LRU_NODE_INFO_T     _clean_lru_node_info;
    LRU_NODE_INFO_T     _clean_lru_node_info_reverse;
    LRU_NODE_INFO_T     _empty_lru_node_info;
    LRU_NODE_INFO_T     _empty_lru_node_info_reverse;
    NODE_CONTAINER_T    _hash_nodes;
    NODE_CONTAINER_T    _lru_nodes;
};

TTC_END_NAMESPACE

#endif
