#ifndef HISTORY_NODE_SIZE_DISTRIBUTE_CMD_H
#define HISTORY_NODE_SIZE_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class HistoryNodeSizeDsitributeCmd: public CProberCmd
{
public:
	HistoryNodeSizeDsitributeCmd(void);
	~HistoryNodeSizeDsitributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(HistoryNodeSizeDsitributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

