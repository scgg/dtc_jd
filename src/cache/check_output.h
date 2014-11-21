/*
 * =====================================================================================
 *
 *       Filename:  check_output.h
 *
 *    Description:  进行检测结果的输出
 *
 *        Version:  1.0
 *        Created:  03/18/2009 12:00:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_OUTPUT_H__
#define __TTC_CHECK_OUTPUT_H__

#include "noncopyable.h"

TTC_BEGIN_NAMESPACE

class CCheckOutput : private noncopyable
{
public:
    CCheckOutput (void);
    ~CCheckOutput (void);

    static CCheckOutput* Instance (void);
    static void Destroy (void);

    int open (void);
    int close (void);

    int output(void);
public:

protected:
protected:

private:
private:
};

TTC_END_NAMESPACE

#endif
