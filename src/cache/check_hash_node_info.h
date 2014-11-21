/*
 * =====================================================================================
 *
 *       Filename:  check_hash_node_info.h
 *
 *    Description:  遍历hash
 *
 *        Version:  1.0
 *        Created:  03/20/2010 11:49:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef __TTC_CHECK_HASH_NODE_INFO_H__
#define __TTC_CHECK_HASH_NODE_INFO_H__

#include "noncopyable.h"
#include "check_interface.h"
#include "hash.h"

#define HASH_NODE_INFO_PRINTER_NAME "hash_node_info_printer"

TTC_BEGIN_NAMESPACE

class CHash;

class CHashPrinter : public ICheck, private noncopyable
{
public:
    CHashPrinter (void);
    virtual ~CHashPrinter (void);

    virtual int open (int argc = 0, char** argv = NULL);
    virtual int close (void);

    virtual const char* name (void)
    {
        return _name;
    }

    virtual int check (void* parameter = NULL);
private:
    uint32_t NG_free(NODE_GROUP_T * NG);
    char    _name[256];
    CHash*  _hash;
};

TTC_END_NAMESPACE

#endif

