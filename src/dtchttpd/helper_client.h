#ifndef HELPER_CLIENT
#define HELPER_CLIENT

#include "pod.h"
#include "thread.h"
#include "helper_config.h"
#include "LockFreeQueue.h"
#include "mysql_manager.h"

namespace dtchttpd
{

class CHelperClient : public CThread
{
public:
	CHelperClient(const char *name, HelperConfig config, LockFreeQueue<MSG> *queue);
	virtual ~CHelperClient();

protected:
	virtual void *Process(void);
	int InsertTask(const MSG &data);
	int ReplaceTask(const MSG &data);
	int CheckTask(const MSG &data);
	int AlarmTask(const MSG &data);
	
private:
	MySqlManager *m_mysql_manager;
	LockFreeQueue<MSG> *m_msg_queue;
};


} //dtchttpd

#endif
