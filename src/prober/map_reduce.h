#ifndef PROBER_MAP_REDUCE_H
#define PROBER_MAP_REDUCE_H

#include "poller.h"
#include "poll_thread.h"
#include "prober_cmd.h"
#include "http_server.h"

class CDispatcher : public CPollerObject {
public:
	CDispatcher();
	CDispatcher(CPollThread *o, int fd);
	~CDispatcher();
	bool Dispatch(CProberCmd *cmd);
	virtual void InputNotify(void);
private:
	int waiting;
};

class CWorker : public CPollerObject {
public:
	CWorker();
	CWorker(CPollThread *o, int fd);
	~CWorker();
	virtual void InputNotify(void);
private:
};

#endif
