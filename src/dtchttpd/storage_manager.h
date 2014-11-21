#ifndef STORAGE_MANAGER
#define STORAGE_MANAGER

#include "LockFreeQueue.h"
#include "pod.h"

namespace dtchttpd
{

class CGroupRouter;
class CHelperGroup;
class CConfig;

class CStorageManager
{
public:
	CStorageManager(CConfig &config);
	virtual ~CStorageManager();
	void Run();
	
private:
	CGroupRouter *m_group_router;
	CHelperGroup **m_helper_group;
	LockFreeQueue<MSG> **m_queue;
	int m_helper_group_num;
	
};

} //dtchttpd

#endif
