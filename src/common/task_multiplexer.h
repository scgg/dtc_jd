#ifndef __H_TASK_MULTIPLEXER_H__
#define __H_TASK_MULTIPLEXER_H__

#include "request_base.h"
#include "multi_request.h"

class CTaskRequest;

class CTaskMultiplexer : public CTaskDispatcher<CTaskRequest>
{
public:
	CTaskMultiplexer(CPollThread *o) : CTaskDispatcher<CTaskRequest>(o), output(o), fastUpdate(0) {};
	virtual ~CTaskMultiplexer(void);
	void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
	void PushTaskQueue(CTaskRequest* req){ output.IndirectNotify(req); }
	void EnableFastUpdate(void) { fastUpdate = 1; }
	
private:
	CRequestOutput<CTaskRequest> output;
	virtual void TaskNotify(CTaskRequest *);
	int fastUpdate;
};

#endif
