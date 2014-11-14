/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords: 特性描述符相关定义
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
 *  $Id: feature.h 7428 2010-10-29 09:26:45Z foryzhou $
 */

/* Change log:
 * 
 * 
 * 
 */

/* Code: */
#ifndef __TTC_FEATURE_H
#define __TTC_FEATURE_H

#include "namespace.h"
#include "global.h"

TTC_BEGIN_NAMESPACE

// feature type
enum feature_id{
	NODE_GROUP = 10, //TTC begin feature id
	NODE_INDEX,
	HASH_BUCKET,
	TABLE_INFO,
	EMPTY_FILTER,
	HOT_BACKUP,
};
typedef enum feature_id FEATURE_ID_T;

struct feature_info{
	uint32_t fi_id;			// feature id
	uint32_t fi_attr;		// feature attribute
	MEM_HANDLE_T fi_handle;		// feature handler
};
typedef struct feature_info FEATURE_INFO_T;

struct base_info{
	uint32_t bi_total;		// total features
	FEATURE_INFO_T bi_features[0];
};
typedef struct base_info BASE_INFO_T;


class CFeature
{
	public:
		static CFeature* Instance();
		static void Destroy();

		MEM_HANDLE_T Handle() const {return M_HANDLE(_baseInfo);}
		const char * Error() const {return _errmsg;}

		int ModifyFeature(FEATURE_INFO_T *fi);
		int DeleteFeature(FEATURE_INFO_T *fi);
		int AddFeature(const uint32_t id, const MEM_HANDLE_T v, const uint32_t attr = 0);
		FEATURE_INFO_T *GetFeatureById(const uint32_t id);

		//创建物理内存并格式化
		int Init(const uint32_t num = MIN_FEATURES);
		//绑定到物理内存
		int Attach(MEM_HANDLE_T handle);
		//脱离物理内存
		int Detach(void);

	public:
		CFeature();
		~CFeature();

	private:
		BASE_INFO_T *_baseInfo;
		char         _errmsg[256];
};

TTC_END_NAMESPACE

#endif
/* ends here */
