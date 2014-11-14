/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords:  实现NodeGroup、Node的释放、分配，时间链表操作函数等
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
 *  $Id: hb_feature.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */
#ifndef __TTC_HB_FEATURE_H
#define __TTC_HB_FEATURE_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "namespace.h"
#include "singleton.h"
#include "global.h"

struct hb_feature_info
{
	int64_t	master_up_time;
	int64_t slave_up_time;
};
typedef struct hb_feature_info HB_FEATURE_INFO_T;

class CHBFeature
{
public:
	CHBFeature();
	~CHBFeature();
		
	static CHBFeature* Instance(){return CSingleton<CHBFeature>::Instance();}
	static void Destroy() { CSingleton<CHBFeature>::Destroy();}
		
	int Init(time_t tMasterUptime);
	int Attach(MEM_HANDLE_T handle);
	void Detach(void);
	
	const char *Error() const {return _errmsg;}
	
	MEM_HANDLE_T Handle() const { return _handle; }
	
	int64_t& MasterUptime() { return _hb_info->master_up_time; }
	int64_t& SlaveUptime() { return _hb_info->slave_up_time; }
	
private:
	HB_FEATURE_INFO_T* _hb_info;	
	MEM_HANDLE_T _handle;
	char _errmsg[256];
};

#endif

