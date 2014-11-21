#include "group_router.h"
#include "log.h"
#include "config.h"

using namespace dtchttpd;

extern LockFreeQueue<MSG> g_Queue;
extern dtchttpd::CConfig g_storage_config;

CGroupRouter::CGroupRouter(const char *name, LockFreeQueue<MSG> **queue) : CThread(name, CThread::ThreadTypeAsync), m_msg_queue(queue)
{	
}

CGroupRouter::~CGroupRouter()
{
	if (NULL != m_router_map)
	{
		delete m_router_map;
		m_router_map = NULL;
	}
}

void CGroupRouter::SetRouterMap(CConfig &config)
{
	m_router_map = new int[config.GetDBNum()];
	for (int i=0; i<config.GetDBNum(); i++)
	{
		m_router_map[i] = config.GetMacheineIndex(i);
	}
}

void* CGroupRouter::Process()
{
	while(false == Stopping())
	{
		MSG data;
		bool ret = g_Queue.DeQueue(data);
		if (true == ret)
		{
			log_info("get data from queue. bid %d curve %d data1 %lld data2 %lld extra %lld datetime %ld", 
					 data.bid, data.curve, data.data1, data.data2, data.extra, data.datetime);
			//insert to correct msg queue
			int queue_index;
			if ((3==data.cmd) || (2==data.cmd))
				queue_index = m_router_map[0];
			else
				queue_index = m_router_map[data.bid%g_storage_config.GetDBMod()];
			log_info("dispatch msg to queue[%d]", queue_index);
			if (false == m_msg_queue[queue_index]->EnQueue(data))
			{
				log_error("inert into queue[%d] fail", queue_index);
				g_Queue.EnQueue(data);
			}
		}
		else
		{
			log_info("get queue element fail, queue size is %u has use %u", g_Queue.QueueSize(), g_Queue.Size());
			sleep(2);
			continue;
		}
	}
	return 0;
}
