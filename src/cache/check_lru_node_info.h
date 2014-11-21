/*
 * =====================================================================================
 *
 *       Filename:  check_lru.h
 *
 *    Description:  检测lru链表的正确性，并输出统计结果
 *
 *        Version:  1.0
 *        Created:  03/18/2009 11:57:47 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang , newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_LRU_NODE_INFO_H__
#define __TTC_CHECK_LRU_NODE_INFO_H__

#include "noncopyable.h"
#include "check_interface.h"
#include "node.h"
#include "tabledef.h"
#include "check_result_info.h"

#define LRU_CHECK_NAME_FORMAT "lru_checker_%d"

TTC_BEGIN_NAMESPACE

class CLRUNodeChecker : public ICheck, private noncopyable
{
public:
    CLRUNodeChecker (int type) : _type(type), _nginfo(NULL), _tabledef(NULL)
    {}
    virtual ~CLRUNodeChecker (void)
    {}

    virtual int open (int argc = 0, char** argv = NULL);
    virtual int close (void);

    virtual const char* name (void)
    {
        return _name;
    }

    virtual int check (void* parameter = NULL);
public:

protected:
protected:

private:
    int common_check (CNode& in_node, const char* direction, lru_node_info_t & node_info);

private:
    int		_type;
    char    _name[256];
    //node group info cache pool hold, get from cache holder
    CNGInfo * _nginfo;
    CTableDefinition * _tabledef;
};

TTC_END_NAMESPACE

#endif
