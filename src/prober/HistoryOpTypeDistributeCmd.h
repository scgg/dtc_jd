#ifndef HISTORY_OP_TYPE_DISTRIBUTE_CMD_H
#define HISTORY_OP_TYPE_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class HistoryOpTypeDistributeCmd: public CProberCmd
{
public:
	HistoryOpTypeDistributeCmd(void);
	~HistoryOpTypeDistributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(HistoryOpTypeDistributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

