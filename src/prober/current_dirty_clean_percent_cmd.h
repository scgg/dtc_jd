#ifndef CURRENT_DIRTY_CLEAN_PERCENT_CMD_H
#define CURRENT_DIRTY_CLEAN_PERCENT_CMD_H
#include "prober_cmd.h"
class CurrentDirtyCleanPercentCmd: public CProberCmd
{
public:
	CurrentDirtyCleanPercentCmd(void);
	~CurrentDirtyCleanPercentCmd(void);
	virtual int GetSize() {	return 1; }
	virtual bool ReplyBody(std::string &resp);
	virtual bool InitAndCheck(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentDirtyCleanPercentCmd)
	DECLARE_CLASS_INFO()
};

#endif

