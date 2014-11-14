#ifndef JOIN_H
#define JOIN_H

#include "thread.h"
#include "sockaddr.h"

class CFDTransferGroup;

class Join : public CThread{
public:
	Join(const char *name, int listenfd, CFDTransferGroup *group);
	virtual ~Join();
	
protected:
	virtual void *Process(void);
	
private:
	int netfd;
	CFDTransferGroup *fdtransfer_group;
};

#endif
