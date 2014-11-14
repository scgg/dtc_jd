#ifndef HISTORY_NODE_SURVIVAL_TIME_DISTRIBUTE_CMD_H
#define HISTORY_NODE_SURVIVAL_TIME_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class HistoryNodeSurvivalTimeDistributeCmd: public CProberCmd
{
public:
	HistoryNodeSurvivalTimeDistributeCmd(void);
	~HistoryNodeSurvivalTimeDistributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(HistoryNodeSurvivalTimeDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

