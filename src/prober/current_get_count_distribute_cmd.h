#ifndef CURRENT_GET_COUNT_DISTRIBUTE_CMD_H
#define CURRENT_GET_COUNT_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentGetCountDistributeCmd: public CProberCmd
{
public:
	CurrentGetCountDistributeCmd(void);
	~CurrentGetCountDistributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentGetCountDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

