#ifndef CURRENT_ROW_SIZE_DISTRIBUTE_CMD_H
#define CURRENT_ROW_SIZE_DISTRIBUTE_CMD_H
#include "prober_cmd.h"
class CurrentRowSizeDsitributeCmd: public CProberCmd
{
public:
	CurrentRowSizeDsitributeCmd(void);
	~CurrentRowSizeDsitributeCmd(void);
	virtual int GetSize();
	virtual bool ReplyBody(std::string &resp);


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentRowSizeDsitributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

