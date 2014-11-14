#include <stdio.h>
#include <string.h>
#include "singleton.h"
#include "feature.h"
#include "global.h"

TTC_USING_NAMESPACE


CFeature* CFeature::Instance()
{
	return CSingleton<CFeature>::Instance();
}

void CFeature::Destroy()
{
	return CSingleton<CFeature>::Destroy();
}


CFeature::CFeature() :
	_baseInfo(NULL)
{
	memset(_errmsg, 0, sizeof(_errmsg));
}

CFeature::~CFeature()
{
}
/* feature id -> feature.  拷贝输入feature 到 找到feature
 */
int CFeature::ModifyFeature(FEATURE_INFO_T *fi)
{
	if(!fi)
		return -1;

	FEATURE_INFO_T *p = GetFeatureById(fi->fi_id);
	if(!p){
		snprintf(_errmsg, sizeof(_errmsg), "not found feature[%d]", fi->fi_id);
		return -2;
	}

	*p = *fi;
	return 0;
}
/* feature id -> feature. 清空这个feature 
 */
int CFeature::DeleteFeature(FEATURE_INFO_T *fi) 
{
	if(!fi)
		return -1;

	FEATURE_INFO_T *p = GetFeatureById(fi->fi_id);
	if(!p)
	{
		snprintf(_errmsg, sizeof(_errmsg), "not found feature[%d]", fi->fi_id);
		return -2;
	}

	//delete feature
	p->fi_id      = 0;
	p->fi_attr    = 0;
	p->fi_handle = INVALID_HANDLE;

	return 0;
}
/* 找一个空闲feature, 赋值 
 */
int CFeature::AddFeature(const uint32_t id, const MEM_HANDLE_T v, const uint32_t attr)
{
	if(INVALID_HANDLE == v)
	{
		snprintf(_errmsg, sizeof(_errmsg), "handle is invalid");
		return -1;
	}

	//find freespace
	FEATURE_INFO_T *p = GetFeatureById(0);
	if(!p)
	{
		snprintf(_errmsg, sizeof(_errmsg), "have no free space to add a new feature");
		return -2;
	}
	
	p->fi_id = id;
	p->fi_attr = attr;
	p->fi_handle = v;

	return 0;
}
/* feature id -> feature. 
 * 1. feature id == 0: 则表示找一个空闲feature.
 * 2. 否则根据feature id 找对应的feature
 */
FEATURE_INFO_T *CFeature::GetFeatureById(const uint32_t fd)
{
	if(!_baseInfo || _baseInfo->bi_total == 0)
	{
		goto EXIT;
	}

	for(uint32_t i = 0; i < _baseInfo->bi_total; i++)
	{
		if(_baseInfo->bi_features[i].fi_id == fd)
		{
			return (&(_baseInfo->bi_features[i]));
		}
	}

EXIT:
	return (FEATURE_INFO_T *)(0);
}
/* 1. 创建num个空feature
 * 2. 初始化头信息(baseInfo)
 */
int CFeature::Init(const uint32_t num)
{
	size_t size = sizeof(FEATURE_INFO_T);
	size *= num;
	size += sizeof(BASE_INFO_T);

	MEM_HANDLE_T v = M_CALLOC(size);
	if(INVALID_HANDLE == v)
	{
		snprintf(_errmsg, sizeof(_errmsg), "init features failed, %s", M_ERROR());
		return -1;
	}

	_baseInfo = M_POINTER(BASE_INFO_T, v);
	_baseInfo->bi_total = num;

	return 0;
}
/* feature已经存在，第一个feature的内存句柄。直接初始化头信息指向 
 */
int CFeature::Attach(MEM_HANDLE_T handle)
{
	if(INVALID_HANDLE == handle)
	{

		snprintf(_errmsg, sizeof(_errmsg), "attach features failed, memory handle=0");
		return -1;
	}

	_baseInfo = M_POINTER(BASE_INFO_T, handle);
	return 0;
}

int CFeature::Detach(void)
{
	_baseInfo   = NULL;
	return 0;
}
