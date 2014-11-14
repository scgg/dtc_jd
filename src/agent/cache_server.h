#ifndef AGENT_CACHE_SERVER_H__
#define AGENT_CACHE_SERVER_H__

#include <string>
#include "poll_thread_group.h"
#include <stdlib.h>
#include <time.h>

class CModule;
class CBackWorker;
class CPollThread;

class CCacheServer {
public:
	CCacheServer(const std::string &name, const std::string &addr,
			CModule *owner, CPollThreadGroup *m_threadgroup,
			int connect_per_ttc, const std::string &hotbak_addr);
	~CCacheServer();

	int HandleRequest(const char *buf, int len, CPollThread *thread);
	int RefCnt() const {
		return m_refCnt;
	}
	int Release() {
		return --m_refCnt;
	}
	int AddRef() {
		return ++m_refCnt;
	}
	const std::string &Name() {
		return m_name;
	}
	const std::string &Addr() {
		return m_addr;
	}
	void SwitchMechine(int ms);
private:
	CBackWorker *m_workers;
	int m_refCnt;
	std::string m_name;
	std::string m_addr;
	CModule *m_module;
	int* m_currentWorkers;
	int m_connect_per_ttc;
	CPollThreadGroup *m_threadgroup;
	std::string m_hotbak_addr;
	CBackWorker *m_master_workers;
	CBackWorker *m_hotbak_workers;

};

struct CCacheServerHolder {
	CCacheServerHolder(CCacheServer *s = 0) :
			m_server(s) {
		if (m_server)
			m_server->AddRef();
	}

	~CCacheServerHolder() {
		if (m_server)
			m_server->Release();
	}

	CCacheServerHolder(const CCacheServerHolder &r) :
			m_server(r.m_server) {
		if (m_server)
			m_server->AddRef();
	}

	CCacheServerHolder &operator =(const CCacheServerHolder &r) {
		CCacheServerHolder tmp(r);
		//swap
		tmp.m_server = m_server;
		m_server = r.m_server;

		return *this;
	}

	CCacheServer *operator ->() const {
		return m_server;
	}

	CCacheServer *Get() const {
		return m_server;
	}

private:
	CCacheServer *m_server;
};

inline bool operator <(const CCacheServerHolder &l,
		const CCacheServerHolder &r) {
	return l->Name() < r->Name();
}

#endif
