/*                               -*- Mode: C -*- 
 * 
 * Keywords:  空节点过滤功能，作为一个feature来注入内存管理
 * 	      目前仅支持对int key做过滤功能，不支持string key
 * 	      按照%30000把4G用户分为30000张表，每张表大致容纳
 * 	      134K用户, 单个表最大为17k的样子，初始化时不分配
 * 	      任何存储空间，一旦表内有用户，则分配表空间，每次
 * 	      表大小按照512byte的步长来增长直到17k。 
 *
 * 	      512 byte的步长是参考了即通每次放号区段最大为0.1G
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
 *  $Id: empty_filter.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */

#ifndef __TTC_EMPTY_FILTER_H
#define __TTC_EMPTY_FILTER_H

#include "namespace.h"
#include "singleton.h"
#include "global.h"

TTC_BEGIN_NAMESPACE

#define DF_ENF_TOTAL	0	/* 0 = unlimited */
#define DF_ENF_STEP	512 	/* byte */
#define DF_ENF_MOD	30000


struct _enf_table
{
	MEM_HANDLE_T t_handle;
	uint32_t     t_size;
};
typedef struct _enf_table ENF_TABLE_T;

struct _empty_node_filter
{
	uint32_t enf_total;		// 占用的总内存
	uint32_t enf_step;		// 表增长步长
	uint32_t enf_mod;		// 分表算子

	ENF_TABLE_T enf_tables[0];	// 位图表
};
typedef struct _empty_node_filter ENF_T;

class CEmptyNodeFilter
{
	public:
		void SET(uint32_t key);
		void CLR(uint32_t key);
		int  ISSET(uint32_t key);

	public:
		/* 0 = use default value */
		int Init(uint32_t total = 0, uint32_t step = 0, uint32_t mod = 0);
		int Attach(MEM_HANDLE_T);
		int Detach(void);

	public:
		CEmptyNodeFilter();
		~CEmptyNodeFilter();
		static CEmptyNodeFilter* Instance(){return CSingleton<CEmptyNodeFilter>::Instance();}
		static void Destroy(){ CSingleton<CEmptyNodeFilter>::Destroy();}
		const char *Error() const {return _errmsg;}
		const MEM_HANDLE_T Handle() const {return M_HANDLE(_enf);}
	private:
		/* 计算表id */
		uint32_t Index(uint32_t key) { return key%_enf->enf_mod; }
		/* 计算表中的位图偏移 */
		uint32_t Offset(uint32_t key) { return  key/_enf->enf_mod; }
	private:
		ENF_T* _enf;
		char   _errmsg[256];
};


TTC_END_NAMESPACE

#endif

/* ends here */
