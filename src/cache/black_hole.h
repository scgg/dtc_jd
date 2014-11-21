#include <task_request.h>

class CBlackHole : public CTaskDispatcher<CTaskRequest>
{
public:
	CBlackHole(CPollThread *o) : CTaskDispatcher<CTaskRequest>(o), output(o) {};
	virtual ~CBlackHole(void);
	void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
private:
	CRequestOutput<CTaskRequest> output;
	virtual void TaskNotify(CTaskRequest *);
};
