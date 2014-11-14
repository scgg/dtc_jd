/*
 * =====================================================================================
 *
 *       Filename:  constant.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/27/2010 06:48:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef AG_CONSTANT__
#define AG_CONSTANT__

#define MAX_FRONT_WORKER_ID ((1<<64) - 1)

#define CACHE_POINTERS_PER_CACHE 100
#define CACHE_POINTER_NAME_FORMAT "%s#%d"

#define ADMIN_CMD_LEASET_LEN (6*sizeof(int))
#define ADMIN_ALL_POINTERS -1

#define CONN_PER_TTC 2
#define RETRY_TIMES 2

#define RECEIVERBUFF_CHECK_COUNTER 300

#endif
