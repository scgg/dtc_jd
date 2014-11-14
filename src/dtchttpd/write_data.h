#ifndef WRITE_DATA_H
#define WRITE_DATA_H

#include "thread.h"

class WriteData : public CThread
{
public:
	WriteData(const char* name);
	virtual ~WriteData();
protected:
	virtual void *Process(void);
};

#endif