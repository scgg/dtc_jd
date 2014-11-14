#ifndef MEMORY_USAGE_CMD_H
#define MEMORY_USAGE_CMD_H
#include "ProberCmd.h"

class CMemoryUsageCmd: public CProberCmd
{
public:
	CMemoryUsageCmd(void);
	~CMemoryUsageCmd(void);
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);


public:
	IMPL_CLASS_OBJECT_CREATE(CMemoryUsageCmd)
	DECLARE_CLASS_INFO()
};

#endif

