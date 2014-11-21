#include <task_request.h>

class CCacheBypass : public CTaskDispatcher<CTaskRequest>
{
public:
	CCacheBypass(CPollThread *o) : CTaskDispatcher<CTaskRequest>(o), output(o) {};
	virtual ~CCacheBypass(void);
	void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
private:
	CRequestOutput<CTaskRequest> output;
	virtual void TaskNotify(CTaskRequest *);
};
