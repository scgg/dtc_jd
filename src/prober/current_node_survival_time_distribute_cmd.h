#ifndef CURRENT_NODE_SURVIVAL_TIME_DISTRIBUTE_CMD_H
#define CURRENT_NODE_SURVIVAL_TIME_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentNodeSurvivalTimeDistributeCmd: public CProberCmd
{
public:
	CurrentNodeSurvivalTimeDistributeCmd(void);
	~CurrentNodeSurvivalTimeDistributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentNodeSurvivalTimeDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif
