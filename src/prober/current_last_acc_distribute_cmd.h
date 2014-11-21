#ifndef CURRENT_LAST_ACC_DISTRIBUTE_CMD_H
#define CURRENT_LAST_ACC_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentLastAccDistributeCmd: public CProberCmd
{
public:
	CurrentLastAccDistributeCmd(void);
	~CurrentLastAccDistributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentLastAccDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

