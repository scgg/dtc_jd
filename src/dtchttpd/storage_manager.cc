#include "storage_manager.h"
#include "config.h"
#include "LockFreeQueue.h"
#include "group_router.h"
#include "helper_group.h"

using namespace dtchttpd;

CStorageManager::CStorageManager(CConfig &config)
{
	m_helper_group_num = config.GetMachineNum(); // 1v1
	m_queue= new LockFreeQueue<MSG>*[m_helper_group_num];
	for (int i=0; i<m_helper_group_num; i++)
	{
		m_queue[i] = new LockFreeQueue<MSG>();
	}
	//
	m_group_router = new CGroupRouter(config.GetGroupRouterName().c_str(), m_queue);
	m_group_router->SetRouterMap(config);
	m_group_router->InitializeThread();
	//
	m_helper_group = new CHelperGroup*[m_helper_group_num];
	char group_name[256];
	for (int i=0; i<m_helper_group_num; i++)
	{
		snprintf(group_name, sizeof(group_name), "%s@%d", config.GetGroupName().c_str(), i);
		m_helper_group[i] = new CHelperGroup(group_name, config.GetHelperGroupConfig(i), m_queue[i]);
	}
}

CStorageManager::~CStorageManager()
{
	if (NULL != m_helper_group)
	{
		for (int i=0; i<m_helper_group_num; i++)
		{
			if (m_helper_group[i])
				delete m_helper_group[i];
		}
		delete m_helper_group;
		m_helper_group = NULL;
	}
	
	if (NULL != m_group_router)
	{
		delete m_group_router;
		m_group_router = NULL;
	}
	
	if (NULL != m_queue)
	{
		for (int i=0; i<m_helper_group_num; i++)
		{
			if (m_queue[i])
				delete m_queue[i];
		}
		delete m_queue;
		m_queue = NULL;
	}
}

void CStorageManager::Run()
{
	for (int i=0; i<m_helper_group_num; i++)
	{
		m_helper_group[i]->Run();
	}
	m_group_router->RunningThread();
}
