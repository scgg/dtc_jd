/*
 * =====================================================================================
 *
 *       Filename:  check_interface.h
 *
 *    Description:  作为所有check行为的接口
 *
 *        Version:  1.0
 *        Created:  03/18/2009 03:05:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __TTC_CHECK_INTERFACE_H__
#define __TTC_CHECK_INTERFACE_H__

#include <stdlib.h>

#include "interface.h"

__INTERFACE__ ICheck
{
public:
    ICheck (void)
    {
    }

    virtual ~ICheck (void)
    {
    }

    /* check init */
    virtual int open (int argc = 0, char** argv = NULL) = 0;

    /* check release */
    virtual int close (void) = 0;

    /* */
    virtual const char* name (void) = 0;

    /* check process */
    virtual int check (void* parameter = NULL) = 0;

public:

protected:
protected:

private:
private:
};

#endif
