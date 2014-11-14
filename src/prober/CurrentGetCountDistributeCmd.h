#ifndef CURRENT_GET_COUNT_DISTRIBUTE_CMD_H
#define CURRENT_GET_COUNT_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class CurrentGetCountDistributeCmd: public CProberCmd
{
public:
	CurrentGetCountDistributeCmd(void);
	~CurrentGetCountDistributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentGetCountDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

