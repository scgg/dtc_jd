#ifndef CURRENT_DIRTY_CLEAN_PERCENT_CMD_H
#define CURRENT_DIRTY_CLEAN_PERCENT_CMD_H
#include "ProberCmd.h"
class CurrentDirtyCleanPercentCmd: public CProberCmd
{
public:
	CurrentDirtyCleanPercentCmd(void);
	~CurrentDirtyCleanPercentCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize() {	return 1; }


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentDirtyCleanPercentCmd)
	DECLARE_CLASS_INFO()
};

#endif

