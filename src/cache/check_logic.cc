/*
 * =====================================================================================
 *
 *       Filename:  check_logic.cc
 *
 *    Description:  check logic impl
 *
 *        Version:  1.0
 *        Created:  03/19/2009 03:54:40 PM
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
#include <stdint.h>

#include "check_logic.h"
#include "singleton.h"
#include "check_code.h"
#include "check_hash_node_info.h"
#include "check_lru_node_info.h"
#include "check_result.h"
#include "check_output.h"
#include "check_cache_holder.h"
#include "log.h"
#include "memcheck.h"

TTC_BEGIN_NAMESPACE

extern int optind;
extern int opterr;

int CCheckLogic::register_checker (void)
{
    int         check_code  = CHECK_UNKNOW;
    ICheck*     checker     = NULL;

    /* init checker */
    _checker.clear ();

    /* init hash printer */
    NEW (CHashPrinter, checker);

    if (NULL == checker)
    {
        log_error ("create hash node info checker failed, error message[%s]", strerror(errno));
        return CHECK_ERROR_LRU;
    }

    /* push clean checker to vector */
    _checker.push_back (checker);

    check_code = checker->open ();
    if (CHECK_SUCC != check_code)
    {
        log_error ("%s", "init hash node info checker failed");
        return check_code;
    }

    for (int type = 0; type < 3; ++type)
    {
        /* init lru checker 0: clean lru; 1:dirty lru; 2:empty lru */
        NEW (CLRUNodeChecker(type), checker);

        if (NULL == checker)
        {
            log_error ("create lru checker[%d] failed, error message[%s]", type, strerror(errno));
            return CHECK_ERROR_LRU;
        }

        /* push clean checker to vector */
        _checker.push_back (checker);

        check_code = checker->open ();
        if (CHECK_SUCC != check_code)
        {
            log_error ("%s", "init lru checker failed");
            return check_code;
        }
    }

    return CHECK_SUCC;
}

CCheckLogic::CCheckLogic (void) : 
    _check_result(NULL), 
    _check_output(NULL), 
    _cache_holder(NULL),
    _need_check(false), 
    _need_fix(false), 
    _need_print(false),
    _shm_key(-1) 
{
}

CCheckLogic::~CCheckLogic (void)
{
}

CCheckLogic* CCheckLogic::Instance (void)
{
    return CSingleton<CCheckLogic>::Instance ();
}

void CCheckLogic::Destroy (void)
{
    return CSingleton<CCheckLogic>::Destroy ();
}

int CCheckLogic::parse_parameter (int argc, char** argv)
{
    int c = -1;

    /* reset opt */
    optind  = 0;
    opterr  = 0;

    for (; (c = getopt (argc, argv, "k:cfp")) != -1;)
    {
        switch (c)
        {
            case 'k':
                _shm_key = atoi (optarg);
               break;

            case 'c':
                _need_check = true;
                break;

            case 'f':
                _need_fix = true;
                break;

            case 'p':
                _need_print = true;
                break;

            default :
                log_error ("parameter[%c] is invalid", c);
                fprintf (stderr, "usage:  ./check_fix_tool -k[shm key] -c[check] -f[fix] -p[print shm info]\n");
                fprintf (stderr, "sample: ./check_fix_tool -k 1778 -c -f -p\n");
                return CHECK_ERROR_PARA;
        }
    }

    if (_shm_key < 0 || (!_need_check && !_need_fix && !_need_print))
    {
        if (_shm_key < 0)
        {
            log_error ("shm key[%d] is invalid", _shm_key);
        }

        fprintf (stderr, "usage:  ./check_fix_tool -k[shm key] -c[check] -f[fix] -p[print shm info]\n");
        fprintf (stderr, "sample: ./check_fix_tool -k 1778 -c -f -p\n");
        return CHECK_ERROR_PARA;
    }

    return CHECK_SUCC;
}

int CCheckLogic::open (int argc, char** argv)
{
    int return_code = CHECK_UNKNOW;

    /* parse parameter */
    return_code = parse_parameter (argc, argv);
    if (CHECK_SUCC != return_code)
    {
        return return_code;
    }

    /* init cache holder */
    _cache_holder = CCacheHolder::Instance ();
    if (NULL == _cache_holder)
    {
        log_error ("create cache holder failed, msg[%s]", strerror(errno));
        return CHECK_ERROR_HOLDER;
    }

    return_code  = _cache_holder->open(_shm_key);
    if (CHECK_SUCC != return_code)
    {
        log_error ("%s", "init check cache holder failed");
        return return_code;
    }

    /* init property */
    return_code = register_checker ();
    if (CHECK_SUCC != return_code)
    {
        return return_code;
    }

    /* init check result */
    _check_result = CCheckResult::Instance ();
    if (NULL == _check_result)
    {
        log_error ("create check result failed, error message[%s]", strerror(errno));
        return CHECK_ERROR_RESULT;
    }

    return_code = _check_result->open ();
    if (CHECK_SUCC != return_code)
    {
        log_error ("%s", "init check result failed");
        return return_code;
    }

    /* init check output */
    _check_output = CCheckOutput::Instance ();
    if (NULL == _check_output)
    {
        log_error ("create check output failed, error message[%s]", strerror(errno));
        return CHECK_ERROR_OUTPUT;
    }

    return_code = _check_output->open ();
    if (CHECK_SUCC != return_code)
    {
        log_error ("%s", "init check output failed");
        return return_code;
    }

    return CHECK_SUCC;
}

int CCheckLogic::close (void)
{
    for (uint32_t i = 0; i < _checker.size(); ++i)
    {
        DELETE(_checker[i]);
        _checker[i] = NULL;
    }

    _checker.clear ();

    if (NULL != _check_result)
    {
        _check_result->close ();
        CCheckResult::Destroy ();
        _check_result = NULL;
    }

    if (NULL != _check_output)
    {
        _check_output->close ();
        CCheckOutput::Destroy ();
        _check_output = NULL;
    }

    if (NULL != _cache_holder)
    {
        _cache_holder->close ();
        CCacheHolder::Destroy ();
        _cache_holder = NULL;
    }

    return CHECK_SUCC;
}

int CCheckLogic::do_check_logic (void)
{
    int return_code = CHECK_SUCC;

    /* check logic */
    if (_need_check || _need_fix || _need_print)
    {
        for (uint32_t i = 0; i < _checker.size(); ++i)
        {
            if (NULL == _checker[i])
            {
                log_error ("the checker[%p] is invalid", _checker[i]);
                return CHECK_ERROR_LOGIC;
            }

            int ret = _checker[i]->check (&_need_fix);
            if (CHECK_SUCC != ret)
            {
                return_code = ret;
            }
        }
    }

    if (_need_print)
    {
        _check_output->output ();
    }

    return return_code;
}

TTC_END_NAMESPACE

