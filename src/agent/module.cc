#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <cstdlib>

#include "front_worker.h"
#include "module.h"
#include "module_acceptor.h"
#include "log.h"
#include "constant.h"
#include "chash.h"
#include "CFdDispacher.h"
#include "stat_agent.h"
#include "dispacher_worker_type.h"
#include "stat_definition.h"

uint32_t CModule::GetNextAvailableWorkerId() {
	while (1) {
		if (m_currentClientId == UINT32_MAX)
			m_currentClientId = 0;
		m_currentClientId++;
		//if (m_frontWorkers.find(m_currentClientId) == m_frontWorkers.end())
		return m_currentClientId;
	}
}

CModule::CModule(uint32_t id, CPollThread *thread,
		CPollThreadGroup *threadgroup, CConfig* gConfigObj,
		CStatManager *statManager, CFdDispacher *fdDispacher) :
		m_id(id), m_acceptor(0), m_thread(thread), m_threadgroup(threadgroup), m_pFdDispacher(
				fdDispacher),m_statManager(statManager), m_gConfigObj(
				gConfigObj) {
	m_accessToken = "";
	darkLaunchPercentage = 0;
	m_curConn = 0;
	m_currentClientId = 0;
	m_frontWorkers =
			new std::map<uint32_t, CFrontWorker *>[threadgroup->GetPollThreadSize()];
	m_cacheServers =
			new std::map<std::string, CCacheServer *>[threadgroup->GetPollThreadSize()];
	m_selector =
			new std::map<uint32_t, CCacheServerHolder>[threadgroup->GetPollThreadSize()];
	m_duplicateKeys =
			new std::map<uint32_t, std::set<CCacheServerHolder> >[threadgroup->GetPollThreadSize()];
}

CModule::~CModule() {
	delete m_acceptor;

//	for (std::map<std::string, CCacheServer *>::iterator iter =
//			m_cacheServers.begin(); iter != m_cacheServers.end(); ++iter) {
//		delete iter->second;
//	}
	DispachCloseFd();
	DispachRelaseAllCacheServer();
	log_debug("~CModule finished!");
}

int CModule::Listen(const std::string &listenOn) {
	m_listenOn = listenOn;
	m_acceptor = new CModuleAcceptor(); //m_threadgroup
	int rtn = m_acceptor->Build(listenOn, m_thread, this, m_threadgroup);
	if (rtn < 0) {
		log_error("listen on %s failed", listenOn.c_str());
		return rtn;
	}

	return 0;
}

int CModule::AddCacheServer(CPollThread * thread,const std::string &name,
	const std::string &addr, int virtualNode,const std::string &hotbak_addr,int mode) {
	int threadId = this->m_threadgroup->GetPollThreadIndex(thread);
	int connect_per_ttc = m_gConfigObj->GetIntVal("agent", "ConnectPerTtc",1);
	log_debug("the connect_per_ttc is %d", connect_per_ttc);
	CCacheServer *s = new CCacheServer(name, addr, this, m_threadgroup,connect_per_ttc,hotbak_addr,mode);
	m_cacheServers[threadId][name] = s;

	if (virtualNode == ADMIN_ALL_POINTERS)
	{
		for (int i = 0; i < CACHE_POINTERS_PER_CACHE; ++i)
		{
			AddVirtualNode(threadId, name, i);
		}
	} else
	{
		if (virtualNode < 0 || virtualNode >= CACHE_POINTERS_PER_CACHE)
		{
			log_error("virtual node index exceeds: %d", virtualNode);
			return -1;
		}
		AddVirtualNode(threadId, name, virtualNode);
	}
	return 0;
}

uint32_t CModule::Hash(const std::string &name, int virtualNode) {
	char buf[16];
	snprintf(buf, sizeof(buf), "%d", virtualNode);
	std::string vname = name + "#" + buf;
	return chash(vname.c_str(), vname.length());
}

void CModule::AddVirtualNode(int threadId, const std::string &name,
		int virtualNode) {
	uint32_t hash = Hash(name, virtualNode);
	std::map<uint32_t, CCacheServerHolder>::iterator i =m_selector[threadId].find(hash);
	if (i != m_selector[threadId].end()) {
		//we are so lucky ...
		if (i->second->Name() < name) {
			m_duplicateKeys[threadId][hash].insert(
					CCacheServerHolder(m_cacheServers[threadId][name]));
		} else {
			m_duplicateKeys[threadId][hash].insert(i->second);
			i->second = CCacheServerHolder(m_cacheServers[threadId][name]);
		}
	} else {
		m_selector[threadId][hash] = CCacheServerHolder(m_cacheServers[threadId][name]);
	}
}

int CModule::RemoveCacheServer(CPollThread * thread, const std::string &name,
		int virtualNode) {
	int threadId = this->m_threadgroup->GetPollThreadIndex(thread);

	if (virtualNode == ADMIN_ALL_POINTERS) {
		for (int i = 0; i < CACHE_POINTERS_PER_CACHE; ++i) {
			RemoveVirtualNode(threadId, name, i , 1);
		}
	} else {
		if (virtualNode < 0 || virtualNode >= CACHE_POINTERS_PER_CACHE) {
			log_error("virtual node index exceeds: %d", virtualNode);
			return -1;
		}
		RemoveVirtualNode(threadId, name, virtualNode , 1);
	}
	return 0;
}

int CModule::RelaseAllCacheServer(CPollThread * thread)
{

	int threadId = this->m_threadgroup->GetPollThreadIndex(thread);
	m_cacheServers[threadId].clear();
	m_selector[threadId].clear();
	m_duplicateKeys[threadId].clear();
	return 0;
}

void CModule::RemoveVirtualNode(int threadId, const std::string &name,
		int virtualNode,int sig) {
	uint32_t hash = Hash(name, virtualNode);
	std::map<uint32_t, CCacheServerHolder>::iterator i =m_selector[threadId].find(hash);
	if (i == m_selector[threadId].end()) {
		log_notice("virtual node %s#%d not exist when removing", name.c_str(),virtualNode);
		return;
	}

	CCacheServer *s = i->second.Get();
	m_selector[threadId].erase(i);
	std::map<uint32_t, std::set<CCacheServerHolder> >::iterator j =m_duplicateKeys[threadId].find(hash);
	if (j != m_duplicateKeys[threadId].end()) {
		if (!j->second.empty()) {
			m_selector[threadId][hash] = *j->second.begin();
			j->second.erase(j->second.begin());
			if (j->second.empty())
				m_duplicateKeys[threadId].erase(j);
		}
	}
	if (s->RefCnt() == 0) {
		const std::string name = s->Name();
		delete s;
		if(sig==1)
		{
			m_cacheServers[threadId].erase(name);
		}
	}
}

int CModule::DispatchRequest(char *packedKey, uint32_t packedKeyLen,
		char *packet, uint32_t packetLen, CPollThread *thread) {
	int threadId = this->m_threadgroup->GetPollThreadIndex(thread);
	uint32_t hash = chash(packedKey, packedKeyLen);
	log_debug("the packedKey is %s", packedKey);
	log_debug("the hash key is %u", hash);
	//已经打开灰度设置，并且满足灰度条件的转向级联的agent
	if (this->darkLaunchPercentage != 0) {
		if ((hash % 100) < this->darkLaunchPercentage) {
			log_debug("send package to Cascade AgentServer!");
			return this->m_agentServer->HandleRequest(packet, packetLen, thread);
		}
	}
	if (m_selector[threadId].empty()) {
		return -1;
	}
	std::map<uint32_t, CCacheServerHolder>::iterator i =
			m_selector[threadId].upper_bound(hash);

	if (i == m_selector[threadId].end())
		i = m_selector[threadId].begin();
	int ret = i->second->HandleRequest(packet, packetLen, thread);
	return ret;
}

int CModule::DispatchResponse(uint64_t frontWorkerId, char *buf, int len,
		CHitInfo &hitInfo, uint64_t pkgSeq, CPollThread *thread) {

	CFrontWorker * fWorker;
	int threadId = this->m_threadgroup->GetPollThreadIndex(thread);
	if (threadId != -1) {
		fWorker = GetFrontWorker(threadId, frontWorkerId);
	} else {
		log_error("no thread number when dispacher response");
		free(buf);
		return 1;
	}
	if (fWorker != NULL) {
		//log_error("thread id :%d,frontWorkerId:%d",threadId,frontWorkerId);
		if (fWorker->SendResponse(buf, len, hitInfo, pkgSeq) < 0) {
			log_debug("front worker send data error");
			free(buf);
		}
	} else {
		log_error("front worker not exist when reply arrive");
		free(buf);
		return 1;
	}
	return 0;
}

int CModule::DispachAcceptFd(int threadId, int fd) {
	RequestTask *dTask = new RequestTask();
	dTask->SetCmd(DISPACHEREQUEST);
	dTask->SetFd(fd);
	dTask->SetModule(this);
	dTask->SetId(this->GetNextAvailableWorkerId());
	//异步模式
	dTask->SetSyncFlag(0);
	__sync_fetch_and_add(&m_curConn, 1);
	return (this->m_pFdDispacher[threadId].Push(dTask));
}

int CModule::DispachCloseFd() {
	CloseFwsTask *dTask = new CloseFwsTask();
	dTask->SetCmd(CLOSEALLFRONTWORKERS);
	dTask->SetModule(this);
	//同步模式
	dTask->SetSyncFlag(1);
	for (int i = 0; i < m_threadgroup->GetPollThreadSize(); i++) {
		this->m_pFdDispacher[i].Push(dTask);
	}
	delete(dTask);
	return 0;
}

int CModule::DispachAddCacheServer(const std::string &name,
		const std::string &addr, int virtualNode,const std::string &hotbak_addr,int mode) {
	log_debug("enter DispachAddCacheServer %s,%s,%d",name.c_str(),addr.c_str(),virtualNode);
	for(int i=0;i<m_threadgroup->GetPollThreadSize();i++)
	{
		if (m_cacheServers[i].find(name) != m_cacheServers[i].end()) {
			log_error("try to add %s all pointers while server exist",name.c_str());
			return -1;
		}
	}
	AddServerTask *dTask = new AddServerTask();
	dTask->SetCmd(ADDCACHESERVER);
	dTask->SetModule(this);
	dTask->SetServerName(name);
	dTask->SetAddress(addr);
	dTask->SetVirtualNode(virtualNode);
	dTask->SetHotBackupAddress(hotbak_addr);
	dTask->SetMode(mode);
	//同步模式
	dTask->SetSyncFlag(1);
	for (int i = 0; i < m_threadgroup->GetPollThreadSize(); i++) {
		this->m_pFdDispacher[i].Push(dTask);
	}
	delete(dTask);
	return 0;
}

int CModule::DispachRemoveCacheServer(const std::string &name,int virtualNode) {
	for(int i=0;i<m_threadgroup->GetPollThreadSize();i++)
	{
		if (m_cacheServers[i].find(name) == m_cacheServers[i].end()) {
			log_error("try to add %s all pointers while server exist",name.c_str());
			return -1;
		}
	}
	RemoveServerTask *dTask = new RemoveServerTask();
	dTask->SetCmd(REMOVECACHESERVER);
	dTask->SetModule(this);
	dTask->SetServerName(name);
	dTask->SetVirtualNode(virtualNode);
	//同步模式
	dTask->SetSyncFlag(1);
	for (int i = 0; i < m_threadgroup->GetPollThreadSize(); i++) {
		this->m_pFdDispacher[i].Push(dTask);
	}
	delete(dTask);
	return 0;
}

int CModule::DispachRelaseAllCacheServer()
{
	RelaseAllServerTask *dTask = new RelaseAllServerTask();
	dTask->SetCmd(RELASEALLCACHESERVERS);
	dTask->SetModule(this);
	//同步模式
	dTask->SetSyncFlag(1);
	for (int i = 0; i < m_threadgroup->GetPollThreadSize(); i++)
	{
		this->m_pFdDispacher[i].Push(dTask);
	}
	delete(dTask);
	return 0;
}

/*
 * 以下函数封装对frontworker的操作
 *
 */
void CModule::AddFrontWorker(CPollThread * thread, CFrontWorker *worker) {
	//m_queuesizestatItem--;
	int threadId = m_threadgroup->GetPollThreadIndex(thread);
	this->m_frontWorkers[threadId][worker->Id()] = worker;
}

CFrontWorker * CModule::GetFrontWorker(int threadId, int id) {
	std::map<uint32_t, CFrontWorker *>::iterator i =
			m_frontWorkers[threadId].find(id);
	if (i != m_frontWorkers[threadId].end()) {
		return i->second;
	} else {
		return NULL;
	}
}

int CModule::ReleseFrontWorker(CPollThread *thread, int id) {
	int threadId = m_threadgroup->GetPollThreadIndex(thread);
	CFrontWorker *worker = GetFrontWorker(threadId, id);
	if (NULL != worker) {
		RemoveFrontWorker(threadId, id);
		delete (worker);
		__sync_fetch_and_sub(&m_curConn, 1);
		return 0;
	} else {
		log_error("no Frontworker exist");
		return -1;
	}
}

int CModule::ReleseAllFrontWorker(CPollThread *thread) {
	int threadId = m_threadgroup->GetPollThreadIndex(thread);
	std::map<uint32_t, CFrontWorker *>::iterator i =m_frontWorkers[threadId].begin();
	for (;i!=m_frontWorkers[threadId].end();i++) {
		delete i->second;
	}
	m_frontWorkers[threadId].clear();
	return 0;
}

void CModule::RemoveFrontWorker(int threadId, uint32_t id) {
	m_frontWorkers[threadId].erase(id);
}

int CModule::GetCurConnection() {
	return m_curConn;
}

CStatManager * CModule::GetStatManager() {
	return this->m_statManager;
}

/*
 * Authir:jasonqiu
 * agent级联服务器设置
 * 当前并不支持admin端口进行操作
 */
int CModule::SetCascadeAgentServer(const std::string &name,
		const std::string &addr, int dkPercent) {
	this->darkLaunchPercentage = dkPercent;
	int connect_per_ttc = m_gConfigObj->GetIntVal("agent", "ConnectPerTtc", 1);
	CCacheServer *s = new CCacheServer(name, addr, this, m_threadgroup,
			connect_per_ttc,"",0);
	this->m_agentServer = CCacheServerHolder(s);
	return 0;
}

/*
 * Dump出module中所有的cacheserver的配置信息
 */
int CModule::DumpModuleConfig(std::string &message)
{
	log_debug("enter DumpModuleConfig");
	for(int i=0;i<(m_threadgroup->GetPollThreadSize());i++)
	{
		std::map<std::string, CCacheServer *>::iterator itor=m_cacheServers[i].begin();
		for(;itor != (m_cacheServers[i].end()); itor++)
		{
			char t[256];
			std::string s;
			sprintf(t,"%d",i);
			s = t;
			//log_error("Thread:%d,CacheServerName:%s,CacheServerAddr:%s",i,itor->second->Name().c_str(),itor->second->Addr().c_str());
			message=message+"     threadname:agentworker"+s+"  CacheServerName:"+itor->second->Name()+"  CacheServerAddr:"+itor->second->Addr()+"\n";
		}
	}
	return 0;
}


int CModule::DispachSwitchCacheServer(std::string name,uint32_t mode) {
	SwitchCacheServerTask *dTask = new SwitchCacheServerTask();
	dTask->SetCmd(SWITCHCACHESERVER);
	dTask->SetModule(this);
	//同步模式
	dTask->SetSyncFlag(1);
	dTask->SetName(name);
	dTask->SetMode(mode);
	for (int i = 0; i < m_threadgroup->GetPollThreadSize(); i++) {
		this->m_pFdDispacher[i].Push(dTask);
	}
	delete(dTask);
	return 0;
}

int CModule::SwitchCacheServer(CPollThread * thread,std::string name,int mode)
{
	int threadId = m_threadgroup->GetPollThreadIndex(thread);
	std::map<std::string, CCacheServer *>::iterator itor=m_cacheServers[threadId].find(name);
	if(itor!=m_cacheServers[threadId].end())
	{
		itor->second->SwitchMechine(mode);
	}
	return 0;
}



