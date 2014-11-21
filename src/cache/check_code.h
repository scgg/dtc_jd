/*
 * =====================================================================================
 *
 *       Filename:  check_code.h
 *
 *    Description:  hash,lru,检测错误代码
 *
 *        Version:  1.0
 *        Created:  03/19/2009 11:20:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_ERROR_CODE_H__
#define __TTC_CHECK_ERROR_CODE_H__

#include "namespace.h"

TTC_BEGIN_NAMESPACE

enum CHECK_ERROR_CODE
{
    CHECK_UNKNOW        = -1,
    CHECK_SUCC          = 0,
    CHECK_ERROR_HASH    = -10000,
    CHECK_ERROR_LRU,
    CHECK_ERROR_RESULT,
    CHECK_ERROR_OUTPUT,
    CHECK_ERROR_LOGIC,
    CHECK_ERROR_PARA,
    CHECK_ERROR_HOLDER
};

TTC_END_NAMESPACE

#endif

