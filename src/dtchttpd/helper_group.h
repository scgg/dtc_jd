#ifndef HELPER_GROUP
#define HELPER_GROUP

#include "pod.h"
#include "LockFreeQueue.h"
#include "helper_config.h"

namespace dtchttpd
{

class CHelperClient;

class CHelperGroup
{
public:
	CHelperGroup(const char *group_name, HelperGroupConfig *config, LockFreeQueue<MSG> *queue);
	virtual ~CHelperGroup();
	void Run();
	
private:
	int m_helper_num;
	CHelperClient **m_helper_client;

};

} //dtchttpd

#endif
