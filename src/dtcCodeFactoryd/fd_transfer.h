#ifndef FD_TRANSFER_H
#define FD_TRANSFER_H

#include "poller.h"

class CPollThread;
class CPollThreadGroup;

class CFDTransfer : public CPollerObject{
public:
	CFDTransfer();
	int AttachPoller(CPollThread *thread);
	int Transfer(int fd);
	void InputNotify();
private:
	int write_fd;
	CPollThread *poll_thread;
};

class CFDTransferGroup{
public:
	CFDTransferGroup(int num);
	void Attach(CPollThreadGroup *pollGroup);
	void Transfer(int fd);
private:
	CFDTransfer *transfer_group;
	int group_count;
};

#endif
