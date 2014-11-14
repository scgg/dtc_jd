/*
 * =====================================================================================
 *
 *       Filename:  check_node_info.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2010 05:26:47 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef _CHECK_RESULT_INFO_H_
#define _CHECK_RESULT_INFO_H_

#include <vector>

#define CHECK_NODE_CLEAN 0
#define CHECK_NODE_DIRTY 1

typedef struct lru_node_info
{
    long long _key;
    NODE_ID_T   _node_id;
    int     _dirty_flag;
    MEM_HANDLE_T _time;
    std::vector<int> _row_flags;

    void clear()
    {
        _node_id = -1;
        _dirty_flag = -1;
        _time = 0;
        _row_flags.clear();
    }
}lru_node_info_t;


typedef struct _error_node_info_
{
	NODE_ID_T       _node_id;
	MEM_HANDLE_T    _mem_handle;
} ERR_NODE_INFO;

#endif
