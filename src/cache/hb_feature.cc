#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "hb_feature.h"
#include "global.h"

TTC_USING_NAMESPACE

CHBFeature::CHBFeature(): _hb_info(NULL), _handle(INVALID_HANDLE)
{
	memset(_errmsg, 0, sizeof(_errmsg));
}

CHBFeature::~CHBFeature()
{

}

int CHBFeature::Init(time_t tMasterUptime)
{
	_handle = M_CALLOC(sizeof(HB_FEATURE_INFO_T));
	if(INVALID_HANDLE == _handle){
		snprintf(_errmsg, sizeof(_errmsg), "init hb_feature fail, %s", M_ERROR());
		return -ENOMEM;
	}
	
	_hb_info =  M_POINTER(HB_FEATURE_INFO_T, _handle);
	_hb_info->master_up_time = tMasterUptime;
	_hb_info->slave_up_time = 0;
	
	return 0;
}

int CHBFeature::Attach(MEM_HANDLE_T handle)
{
	if(INVALID_HANDLE == handle){
        snprintf(_errmsg, sizeof(_errmsg), "attach hb feature failed, memory handle = 0");
		return -1;
    }

	_handle = handle;
    _hb_info =  M_POINTER(HB_FEATURE_INFO_T, _handle);
	
    return 0;
}

void CHBFeature::Detach(void)
{
	_hb_info = NULL;
	_handle = INVALID_HANDLE;
}

