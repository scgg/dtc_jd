#ifndef AGENT_MODULE_H__
#define AGENT_MODULE_H__

#include <string>
#include <map>
#include <set>

#include <stdint.h>
#include "cache_server.h"
#include "hitinfo.h"
#include "poll_thread_group.h"
#include "StatMgr.h"

class CModuleAcceptor;
class CFrontWorker;
class CFdDispacher;

class CModule {
public:
	CModule(uint32_t id, CPollThread *thread, CPollThreadGroup *threadgroup,
			CConfig* gConfigObj, CStatManager *statManager,
			CFdDispacher *fdDispacher);
	~CModule();

	uint32_t Id() {
		return m_id;
	}

	int Listen(const std::string &listenOn);

	/* admin*/
	int AddCacheServer(CPollThread * thread,const std::string &name,
		const std::string &addr, int virtualNode,const std::string &hotbak_addr,int mode);
	int RemoveCacheServer(CPollThread * thread, const std::string &name,
			int virtualNode);

	/* interface */
	int DispatchRequest(char *packedKey, uint32_t packedKeyLen, char *packet,
			uint32_t packetLen, CPollThread *thread);
	int DispatchResponse(uint64_t frontWorkerId, char *buf, int len,
			CHitInfo &hitInfo, uint64_t pkgSeq, CPollThread *thread);
	const std::string &GetListenAddr() const {
		return m_listenOn;
	}
	const std::string &GetAccessToken() const {
		return m_accessToken;
	}
	void SetAccessToken(std::string accessToken) {
		m_accessToken = accessToken;
	}
	int SetCascadeAgentServer(const std::string &name, const std::string &addr,
			int dkPercent);
	int DispachAcceptFd(int threadId, int fd);
	CStatManager *GetStatManager();
	int GetCurConnection();
	void AddFrontWorker(CPollThread * thread, CFrontWorker *worker);
	CFrontWorker *GetFrontWorker(int threadId, int id);
	void RemoveFrontWorker(int threadId, uint32_t id);
	int ReleseFrontWorker(CPollThread *thread, int id);
	uint32_t GetNextAvailableWorkerId();
	int DispachCloseFd();
	int ReleseAllFrontWorker(CPollThread *thread);
	int DispachAddCacheServer(const std::string &name,
			const std::string &addr, int virtualNode,const std::string &hotbak_addr,int mode);
	int DispachRemoveCacheServer(const std::string &name, int virtualNode);
	int DispachRelaseAllCacheServer();
	int RelaseAllCacheServer(CPollThread * thread);
	int DumpModuleConfig(std::string &message);
	int DispachSwitchCacheServer(std::string name,uint32_t mode);
	int SwitchCacheServer(CPollThread * thread,std::string name,int mode);
private:
	uint32_t Hash(const std::string &name, int virtualNode);
	void AddVirtualNode(int threadId, const std::string &name, int virtualNode);
	void RemoveVirtualNode(int threadId, const std::string &name,
			int virtualNode, int sig);

	uint32_t m_id;
	std::string m_listenOn;
	CModuleAcceptor *m_acceptor;

	std::map<uint32_t, CFrontWorker *> *m_frontWorkers;
	std::map<std::string, CCacheServer *> *m_cacheServers;
	std::map<uint32_t, CCacheServerHolder> *m_selector;
	std::map<uint32_t, std::set<CCacheServerHolder> > *m_duplicateKeys;

	CPollThread *m_thread;
	CPollThreadGroup *m_threadgroup;
	std::string m_accessToken;
	CFdDispacher *m_pFdDispacher;
	CStatManager *m_statManager;
	//为了使用agent级联新添加变量，使用原cacheserver
	CCacheServerHolder m_agentServer;
	int darkLaunchPercentage;
	CConfig* m_gConfigObj;
	volatile uint32_t m_curConn;
	int m_worknumber;
	uint32_t m_currentClientId;
};

#endif
