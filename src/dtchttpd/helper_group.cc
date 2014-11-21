#include "helper_group.h"
#include "helper_client.h"

using namespace dtchttpd;

CHelperGroup::CHelperGroup(const char *group_name, HelperGroupConfig *config, LockFreeQueue<MSG> *queue)
{
	m_helper_num = config->helper_num;
	m_helper_client = new CHelperClient*[m_helper_num];
	char thread_name[256];
	for (int i=0; i<m_helper_num; i++)
	{
		snprintf(thread_name, sizeof(thread_name), "%s@%d", group_name, i);
		m_helper_client[i] = new CHelperClient(thread_name, config->helper_config, queue);
		m_helper_client[i]->InitializeThread();
	}
}

CHelperGroup::~CHelperGroup()
{
	if (NULL != m_helper_client)
	{
		for (int i=0; i<m_helper_num; i++)
		{
			if (m_helper_client[i])
			{
				m_helper_client[i]->interrupt();
				delete m_helper_client[i];
			}
		}
		delete m_helper_client;
		m_helper_client = NULL;
	}
}

void CHelperGroup::Run()
{
	if (NULL == m_helper_client)
		return;
	for (int i=0; i<m_helper_num; i++)
	{
		m_helper_client[i]->RunningThread();
	}
}
