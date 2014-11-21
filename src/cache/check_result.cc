/*
 * =====================================================================================
 *
 *       Filename:  check_result.cc
 *
 *    Description:  check result impl
 *
 *        Version:  1.0
 *        Created:  03/18/2009 06:12:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "check_result.h"
#include "singleton.h"
#include "check_code.h"

TTC_BEGIN_NAMESPACE

CCheckResult::CCheckResult (void) : _next_count(0), _prev_count(0)
{

}

CCheckResult::~CCheckResult (void)
{

}

CCheckResult* CCheckResult::Instance (void)
{
    return CSingleton<CCheckResult>::Instance ();
}

void CCheckResult::Destroy (void)
{
    return CSingleton<CCheckResult>::Destroy ();
}

int CCheckResult::open (void)
{
    _hash_nodes.clear ();
    _lru_nodes.clear ();

    return CHECK_SUCC;
}

int CCheckResult::close (void)
{
    _next_count = 0;
    _prev_count = 0;
    _hash_nodes.clear ();
    _lru_nodes.clear ();

    return CHECK_SUCC;
}

int CCheckResult::save_dirty_lru_node_info (lru_node_info_t & node_info)
{
    _dirty_lru_node_info.push_back (node_info);
    return CHECK_SUCC;
}

int CCheckResult::save_dirty_lru_node_info_reverse (lru_node_info_t & node_info)
{
    _dirty_lru_node_info_reverse.push_back (node_info);

    return CHECK_SUCC;
}

int CCheckResult::save_clean_lru_node_info (lru_node_info_t & node_info)
{
    _clean_lru_node_info.push_back (node_info);
    return CHECK_SUCC;
}

int CCheckResult::save_clean_lru_node_info_reverse (lru_node_info_t & node_info)
{
    _clean_lru_node_info_reverse.push_back (node_info);

    return CHECK_SUCC;
}

int CCheckResult::save_empty_lru_node_info (lru_node_info_t & node_info)
{
    _empty_lru_node_info.push_back (node_info);

    return CHECK_SUCC;
}
int CCheckResult::save_empty_lru_node_info_reverse (lru_node_info_t & node_info)
{
    _empty_lru_node_info_reverse.push_back (node_info);

    return CHECK_SUCC;
}

//ill node id and this node's mem
int CCheckResult::save_hash_handle (const NODE_ID_T node_id, const MEM_HANDLE_T mem_handle)
{
    ERR_NODE_INFO node_info;

    node_info._node_id     = node_id;
    node_info._mem_handle  = mem_handle;

    //save ill node is and its mem
    _hash_nodes.push_back (node_info);

    return CHECK_SUCC;
}

//ill node id and this node's mem
int CCheckResult::save_lru_handle (const NODE_ID_T node_id, const MEM_HANDLE_T mem_handle)
{
    ERR_NODE_INFO node_info;

    node_info._node_id     = node_id;
    node_info._mem_handle  = mem_handle;

    _lru_nodes.push_back (node_info);

    return CHECK_SUCC;
}

int64_t CCheckResult::node_count_diff (void)
{
    return _next_count - _prev_count;
}

TTC_END_NAMESPACE

