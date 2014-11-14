/*
 * =====================================================================================
 *
 *       Filename:  ttc_global.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/18/2010 10:28:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _TTC_GLOBAL_H_
#define _TTC_GLOBAL_H_
#include "noncopyable.h"

#define TABLE_CONF_NAME  "../conf/table.conf"
#define CACHE_CONF_NAME  "../conf/cache.conf"
#define ALARM_CONF_FILE  "../conf/dtcalarm.conf"
class CTTCGlobal : private noncopyable
{
    public:
        CTTCGlobal (void);
        ~CTTCGlobal (void);
    public:
        static int  _pre_alloc_NG_num;
        static int  _min_chunk_size;
        static int  _pre_purge_nodes;
};
#endif
