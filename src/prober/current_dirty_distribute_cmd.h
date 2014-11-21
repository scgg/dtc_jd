#ifndef CURRENT_DIRTY_DISTRIBUTE_CMD_H
#define CURRENT_DIRTY_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentDirtyDistributeCmd: public CProberCmd
{
public:
	CurrentDirtyDistributeCmd(void);
	~CurrentDirtyDistributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);
	virtual bool InitAndCheck(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentDirtyDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

