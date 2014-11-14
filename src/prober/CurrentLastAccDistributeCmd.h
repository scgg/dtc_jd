#ifndef CURRENT_LAST_ACC_DISTRIBUTE_CMD_H
#define CURRENT_LAST_ACC_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class CurrentLastAccDistributeCmd: public CProberCmd
{
public:
	CurrentLastAccDistributeCmd(void);
	~CurrentLastAccDistributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentLastAccDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

