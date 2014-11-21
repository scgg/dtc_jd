#include "log.h"

#include "back_worker.h"
#include "cache_server.h"
#include "constant.h"

CCacheServer::CCacheServer(const std::string &name, const std::string &addr,
		CModule *owner, CPollThreadGroup *threadgroup, int connect_per_ttc,
		const std::string &hotbak_addr,int mode) :
		m_refCnt(0), m_name(name), m_addr(addr), m_module(owner), m_threadgroup(
				threadgroup), m_hotbak_addr(hotbak_addr),m_mode(mode) {
	int threadNum = m_threadgroup->GetPollThreadSize();
	this->m_connect_per_ttc = connect_per_ttc;
	int CONN_NUM = m_connect_per_ttc * threadNum;
	m_master_workers = new CBackWorker[CONN_NUM];
	m_hotbak_workers = new CBackWorker[CONN_NUM];

	int j = 0;
	for (int i = 0; i < CONN_NUM; ++i) {
		if (i != 0 && i % m_connect_per_ttc == 0) {
			j++;
		}
		if (m_master_workers[i].Build(addr, threadgroup->GetPollThread(j), m_module)< 0) {
			log_crit("build back worker %s for %s failed", addr.c_str(),name.c_str());
			break;
		}
		if (m_hotbak_addr != "") {
			if (m_hotbak_workers[i].Build(m_hotbak_addr,
					threadgroup->GetPollThread(j), m_module) < 0) {
				log_crit("build back worker %s for %s failed", addr.c_str(),
						name.c_str());
				break;
			}
		} else {
			if (m_hotbak_workers[i].Build(m_addr, threadgroup->GetPollThread(j),
					m_module) < 0) {
				log_crit("build hotback back worker %s for %s failed", addr.c_str(),
						name.c_str());
				break;
			}
		}

	}
	m_workers=m_master_workers;
	m_currentWorkers = new int[threadNum];
	for (int i = 0; i < threadNum; i++) {
		m_currentWorkers[i] = 0;
	}
}

CCacheServer::~CCacheServer() {
	delete[] m_currentWorkers;
	delete[] m_master_workers;
	delete[] m_hotbak_workers;
}

void CCacheServer::SwitchMechine(int ms)
{
	if(ms==0)
	{
		log_debug("switch mechine to master");
		m_workers=m_master_workers;
	}
	else
	{
		log_debug("switch mechine to slave");
		m_workers=m_hotbak_workers;
	}
}

int CCacheServer::HandleRequest(const char *buf, int len, CPollThread *thread) {

	int idx = m_threadgroup->GetPollThreadIndex(thread);
	log_debug("the idx is :%d", idx);
	int retry = 0;
//	srand((int) time(0));
	while (retry < RETRY_TIMES) {
		int conn = m_connect_per_ttc * idx + m_currentWorkers[idx];
		log_debug("the conn is :%d", conn);
		if (!m_workers[conn].GetReady()
				|| m_workers[conn].HandleRequest(buf, len) < 0) {
			++retry;
		} else {
			m_currentWorkers[idx]++;
			if (m_currentWorkers[idx] == m_connect_per_ttc) {
				m_currentWorkers[idx] = 0;
			}
			return 0;
		}
		m_currentWorkers[idx]++;
		if (m_currentWorkers[idx] == m_connect_per_ttc) {
			m_currentWorkers[idx] = 0;
		}
	}
	return -1;
}
