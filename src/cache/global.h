/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords:  共有函数，宏定义等
 * 
 *       This is unpublished proprietary
 *       source code of Tencent Ltd.
 *       The copyright notice above does not
 *       evidence any actual or intended
 *       publication of such source code.
 *
 *       NOTICE: UNAUTHORIZED DISTRIBUTION,ADAPTATION OR 
 *       USE MAY BE SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *  $Id: global.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */

#ifndef __TTC_GLOBAL_H
#define __TTC_GLOBAL_H

#include <stdint.h>
#include <stdarg.h>
#include "namespace.h"
#include "bin_malloc.h"

TTC_BEGIN_NAMESPACE

/* 共享内存操作定义 */
#define M_HANDLE(ptr) 		CBinMalloc::Instance()->Handle(ptr)
#define M_POINTER(type, v)	CBinMalloc::Instance()->Pointer<type>(v) 
#define M_MALLOC(size)		CBinMalloc::Instance()->Malloc(size)
#define M_CALLOC(size)		CBinMalloc::Instance()->Calloc(size)
#define M_REALLOC(v, size)      CBinMalloc::Instance()->ReAlloc(v, size)
#define M_FREE(v)		CBinMalloc::Instance()->Free(v)
#define M_ERROR()		CBinMalloc::Instance()->GetErrMsg()

/* Node查找函数 */
#define I_SEARCH(id)		CNodeIndex::Instance()->Search(id)
#define I_INSERT(node)		CNodeIndex::Instance()->Insert(node) 
/*#define I_DELETE(node)		CNodeIndex::Instance()->Delete(node) */

/* memory handle*/
#define MEM_HANDLE_T ALLOC_HANDLE_T

/*Node ID*/
#define NODE_ID_T   		uint32_t
#define INVALID_NODE_ID		((NODE_ID_T)(-1))
#define SYS_MIN_NODE_ID		((NODE_ID_T)(0))
#define SYS_DIRTY_NODE_INDEX	0
#define SYS_CLEAN_NODE_INDEX    1
#define SYS_EMPTY_NODE_INDEX    2
#define SYS_DIRTY_HEAD_ID	(SYS_MIN_NODE_ID + SYS_DIRTY_NODE_INDEX)
#define SYS_CLEAN_HEAD_ID	(SYS_MIN_NODE_ID + SYS_CLEAN_NODE_INDEX)
#define SYS_EMPTY_HEAD_ID	(SYS_MIN_NODE_ID + SYS_EMPTY_NODE_INDEX)

/* CNode time list */
#define LRU_PREV		(0)
#define LRU_NEXT		(1)

/* features */
#define MIN_FEATURES	32

/*Hash ID*/
#define HASH_ID_T   uint32_t

/* Node Group */
#define NODE_GROUP_INCLUDE_NODES 256

/* output u64 format */
#if __WORDSIZE == 64
# define UINT64FMT "%lu"
#else
# define UINT64FMT "%llu"
#endif

#if __WORDSIZE == 64
# define INT64FMT "%ld"
#else
# define INT64FMT "%lld"
#endif

TTC_END_NAMESPACE

#endif
/* ends here */
