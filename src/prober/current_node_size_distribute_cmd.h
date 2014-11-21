#ifndef CURRENT_NODE_SIZE_DISTRIBUTE_CMD_H
#define CURRENT_NODE_SIZE_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentNodeSizeDsitributeCmd: public CProberCmd
{
public:
	CurrentNodeSizeDsitributeCmd(void);
	~CurrentNodeSizeDsitributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentNodeSizeDsitributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

