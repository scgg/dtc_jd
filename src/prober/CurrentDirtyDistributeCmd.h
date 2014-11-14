#ifndef CURRENT_DIRTY_DISTRIBUTE_CMD_H
#define CURRENT_DIRTY_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class CurrentDirtyDistributeCmd: public CProberCmd
{
public:
	CurrentDirtyDistributeCmd(void);
	~CurrentDirtyDistributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentDirtyDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

