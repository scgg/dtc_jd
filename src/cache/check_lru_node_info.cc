/*
 * =====================================================================================
 *
 *       Filename:  check_lru.cc
 *
 *    Description:  check lru impl
 *
 *        Version:  1.0
 *        Created:  03/18/2009 05:33:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang, newmanwnag@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <string.h>

#include "check_lru_node_info.h"
#include "check_cache_holder.h"
#include "check_code.h"
#include "check_result.h"
#include "check_result_info.h"
#include "dbconfig.h"
#include "raw_data.h"
#include "global.h"

TTC_BEGIN_NAMESPACE

int CLRUNodeChecker::open (int argc, char** argv)
{
    const char * table_conf = "../conf/table.conf";

    CDbConfig * dbconfig = NULL;
    dbconfig = CDbConfig::Load(table_conf);
    if(!dbconfig)
    {
        log_error ("new dbconfig error");
        return  CHECK_ERROR_LRU;
    }

    _tabledef = dbconfig->BuildTableDefinition();
    if(!_tabledef)
    {
        log_error ("make table definition error");
        return  CHECK_ERROR_LRU;
    }

    snprintf(_name, sizeof(_name) - 1, LRU_CHECK_NAME_FORMAT, _type);

    //get node group info from cache pool
    _nginfo = CCacheHolder::Instance()->get_nginfo ();
    if (NULL == _nginfo)
    {
        log_error ("%s", "get nginfo's instance failed");
        return CHECK_ERROR_LRU;
    }

    return 0;
}

int CLRUNodeChecker::close (void)
{
    /* clean name */
    memset (_name, 0x00, sizeof(_name));

    /* clean nginfo */
    _nginfo = NULL;

    return 0;
}

int CLRUNodeChecker::common_check (CNode& in_node, const char* direction, lru_node_info_t & node_info)
{
    int err = 0;
    CNode&          node        = in_node;
    NODE_ID_T       node_id     = in_node.NodeID ();
    NODE_ID_T       min_node_id = _nginfo->MinValidNodeID ();
    NODE_ID_T       max_node_id = _nginfo->MaxNodeID ();
    MEM_HANDLE_T    mem_handle  = INVALID_HANDLE;

    //1. check
    mem_handle = node.VDHandle ();
    if (INVALID_HANDLE == mem_handle)
    {
        log_error ("%s::lru's node_id[%u] handle["UINT64FMT"] is invalid ", direction, node_id, mem_handle);
        return CHECK_SUCC;
    }

    if (INVALID_NODE_ID == node_id)
    {
        log_error ("%s::lru's node_id[%u] is invalid, range[%u - %u]", direction, node_id, min_node_id, max_node_id);
        return CHECK_ERROR_LRU;
    }

    if (node_id < min_node_id || node_id > max_node_id)
    {
        log_error ("%s::lru's node_id[%u] is invalid, range[%u - %u]", direction, node_id, min_node_id, max_node_id);
        return CHECK_ERROR_LRU;
    }

    if (node.NotInLruList())
    {
        log_error ("%s::lru's node_id[%u] not in lru list, next[%u], prev[%u]", direction, node_id, node.LruNext(), node.LruPrev());
        return CHECK_ERROR_LRU;
    }

    if (CBinMalloc::Instance()->HandleIsValid(mem_handle) != 0)
    {
        log_error ("%s::lru's node_id[%u] handle["UINT64FMT"] is invalid ", direction, node_id, mem_handle);
        return  CHECK_ERROR_LRU;
    }

    //2. save result
    node_info._node_id = node_id;
    if(node.NextNodeID() == (INVALID_NODE_ID-1))
    {
        node_info._time = node.VDHandle();
        return CHECK_SUCC;
    }

    unsigned char row_flag = -1;
    int row_cnt = -1;
    CRawData raw_data(CBinMalloc::Instance(), 0);
    CRowValue one_row(_tabledef);

    node_info._dirty_flag = node.IsDirty() ? CHECK_NODE_DIRTY : CHECK_NODE_CLEAN;
    
    err = raw_data.Attach(node.VDHandle(), _tabledef->KeyFields() - 1, _tabledef->KeyFormat());
    if(err)
    {
        log_error("lru %d list node %u, attach local raw data to node error, error message: %s ", _type, node_id, raw_data.GetErrMsg());   
        return  CHECK_ERROR_LRU;
    }

    node_info._key = *(int *)raw_data.Key();

    row_cnt = raw_data.TotalRows();
    for(int i = 0 ; i < row_cnt ; i++)
    {
        err = raw_data.DecodeRow(one_row, row_flag, 0);
        if(err)
        {
            log_error("%s::lru's node_id[%u], decode row %d error", direction, node_id, i);
            return  CHECK_ERROR_LRU;
        }

        node_info._row_flags.push_back(row_flag);
    }

    return CHECK_SUCC;
}

int CLRUNodeChecker::check (void* parameter)
{
    CNode header;
    //dirty node head lru list
    switch(_type)
    {
	case 0:
		header = _nginfo->DirtyNodeHead ();
		break;
	case 1:
		header = _nginfo->CleanNodeHead ();
		break;
	case 2:
		header = _nginfo->EmptyNodeHead ();
		break;
	default:
		log_error("wrong type of lru list");
		return CHECK_ERROR_LRU;
	
    }

    uint64_t        next_check_count    = 0;
    uint64_t        prev_check_count    = 0;
    int             ret_code            = CHECK_SUCC;
    lru_node_info_t node_info;

    //first node in dirty lru list
    CNode           iter                = header.Next();

    /* next check */
    //all node in dirty lru list
    while (!(!iter) && (iter != header))
    {
        next_check_count++;

        node_info.clear();
        int ret = common_check (iter, "next", node_info);
        if (CHECK_SUCC != ret)
        {
            ret_code = ret;
            break;
        }

        switch(_type)
        {
        case 0:
            CCheckResult::Instance()->save_dirty_lru_node_info(node_info);
            break;
        case 1:
            CCheckResult::Instance()->save_clean_lru_node_info(node_info);
            break;
        case 2:
            CCheckResult::Instance()->save_empty_lru_node_info(node_info);
            break;
        default:
            log_error("wrong type of lru list");
            return CHECK_ERROR_LRU;
        }

        iter = iter.Next ();
    }

    log_error ("lru list %d check is finished.", _type);
    /* set next node count */
    CCheckResult::Instance()->set_next_count (next_check_count);

    iter   = header.Prev ();

    /* prev check */
    while (!(!iter) && (iter != header))
    {
        prev_check_count++;
        
        node_info.clear();
        int ret = common_check (iter, "prev", node_info);
        if (CHECK_SUCC != ret)
        {
            ret_code = ret;
            break;
        }

        switch(_type)
        {
        case 0:
            CCheckResult::Instance()->save_dirty_lru_node_info_reverse(node_info);
            break;
        case 1:
            CCheckResult::Instance()->save_clean_lru_node_info_reverse(node_info);
            break;
        case 2:
            CCheckResult::Instance()->save_empty_lru_node_info_reverse(node_info);
            break;
        default:
            log_error("wrong type of lru list");
            return CHECK_ERROR_LRU;
        }

        iter = iter.Prev ();
    }

    /* set next node count */
    //how many node checked on clean node head list, back check
    CCheckResult::Instance()->set_prev_count (prev_check_count);

    log_error ("lru list %d reverse check is finished.", _type);

    return ret_code;
}

TTC_END_NAMESPACE
