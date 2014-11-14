#ifndef CURRENT_NODE_SIZE_DISTRIBUTE_CMD_H
#define CURRENT_NODE_SIZE_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class CurrentNodeSizeDsitributeCmd: public CProberCmd
{
public:
	CurrentNodeSizeDsitributeCmd(void);
	~CurrentNodeSizeDsitributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentNodeSizeDsitributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

