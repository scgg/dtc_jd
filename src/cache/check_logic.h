/*
 * =====================================================================================
 *
 *       Filename:  check_logic.h
 *
 *    Description:  组合hash, lru, result, output逻辑
 *
 *        Version:  1.0
 *        Created:  03/19/2009 03:36:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_LOGIC_H__
#define __TTC_CHECK_LOGIC_H__

#include <vector>

#include "noncopyable.h"
#include "interface.h"

TTC_BEGIN_NAMESPACE

__INTERFACE__   ICheck;
class           CCheckResult;
class           CCheckOutput;
class           CCacheHolder;

class CCheckLogic : private noncopyable
{
public:
    CCheckLogic (void);
    ~CCheckLogic (void);

    static CCheckLogic* Instance (void);
    static void Destroy (void);

    int open (int argc, char** argv);
    int close (void);

    int do_check_logic (void);
public:

protected:
protected:

private:
    int register_checker (void);
    int parse_parameter (int argc, char** argv);

private:
    typedef std::vector<ICheck*> CHECK_OBJ_T;

    CHECK_OBJ_T     _checker;
    CCheckResult*   _check_result;
    CCheckOutput*   _check_output;
    CCacheHolder*   _cache_holder;
    bool            _need_check;
    bool            _need_fix;
    bool            _need_print;
    int             _shm_key;
};

TTC_END_NAMESPACE

#endif
