#ifndef CURRENT_ROW_SIZE_DISTRIBUTE_CMD_H
#define CURRENT_ROW_SIZE_DISTRIBUTE_CMD_H
#include "ProberCmd.h"
class CurrentRowSizeDsitributeCmd: public CProberCmd
{
public:
	CurrentRowSizeDsitributeCmd(void);
	~CurrentRowSizeDsitributeCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();


public:
	IMPL_CLASS_OBJECT_CREATE(CurrentRowSizeDsitributeCmd)
	DECLARE_CLASS_INFO()
};

#endif

