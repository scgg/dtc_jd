#ifndef GROUP_ROUTER
#define GROUP_ROUTER

#include "pod.h"
#include "thread.h"
#include "LockFreeQueue.h"

namespace dtchttpd
{

class CConfig;

class CGroupRouter : public CThread
{
public:
	CGroupRouter(const char *name, LockFreeQueue<MSG> **queue);
	virtual ~CGroupRouter();
	void SetRouterMap(CConfig &config);
	
protected:
	virtual void *Process(void);
	
private:
	LockFreeQueue<MSG> **m_msg_queue;
	int *m_router_map;
	
};

} //dtchttpd

#endif
