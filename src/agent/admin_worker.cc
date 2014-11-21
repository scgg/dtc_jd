#include <cstdlib>
#include <errno.h>

#include "tinyxml.h"
#include "poll_thread.h"
#include "admin_worker.h"
#include "admin.h"
#include "admin_protocol.pb.h"
#include "business_module.h"
#include "log.h"
#include "module.h"
#include "constant.h"
#include "config_file.h"
#include "stat_agent.h"
#include "mtpqueue_nolock.h"
#include "dispacher_worker_type.h"

CAdminWorker::CAdminWorker(int fd, CPollThread *thread,
		CBussinessModuleMgr *mgr, CPollThreadGroup *threadgroup,
		CConfig* gConfigObj, CFdDispacher *gFdDispacher) {
	netfd = fd;
	m_moduleMgr = mgr;
	m_gConfigObj = gConfigObj;
	m_sender = new CAGSender(fd, 1024);
	m_sender->Build();
	m_ownerThread = thread;
	m_threadgroup = threadgroup;
	m_gFdDispacher = gFdDispacher;
	EnableInput();
	AttachPoller(thread);
}

void CAdminWorker::InputNotify() {
	if (m_offset < sizeof(ProtocolHeader)) {
		char *buf = (char *) &m_header;
		int rtn = recv(netfd, buf + m_offset, sizeof(ProtocolHeader) - m_offset,
				0);
		if (rtn < 0) {
			if (errno == EAGAIN || errno == EINTR || errno == EINPROGRESS)
				return;

			log_error("admin worker recv error: %m");
			delete this;
			return;
		}

		if (rtn == 0) {
			delete this;
			return;
		}

		m_offset += rtn;
	}

	if (m_offset < sizeof(ProtocolHeader))
		//should not
		return;

	if (m_header.magic != ADMIN_PROTOCOL_MAGIC) {
		log_error("admin magic invalid: %x, should be %x", m_header.magic,
				ADMIN_PROTOCOL_MAGIC);
		delete this;
		return;
	}

	//the length of a request should not exceeds 64M
	if (m_header.length > 64 * 1024 * 1024) {
		log_error("invalid length: %u", m_header.length);
		delete this;
		return;
	}

	if (m_header.cmd >= AC_CmdEnd) {
		log_error("invalid admin cmd received: %u", m_header.cmd);
		delete this;
		return;
	}

	m_request.resize(m_header.length);

	char *buf = &m_request[0];
	int rtn = recv(netfd, buf + m_offset - sizeof(ProtocolHeader),
			m_header.length - m_offset + sizeof(ProtocolHeader), 0);
	if (rtn < 0) {
		if (EAGAIN == errno || EINTR == errno || EINPROGRESS == errno)
			return;
		log_error("admin worker recv error: %m");
		delete this;
		return;
	}

	if (rtn == 0) {
		delete this;
		return;
	}

	m_offset += rtn;
	if (m_offset == sizeof(ProtocolHeader) + m_header.length) {
		m_offset = 0;
		ProcessRequest();
	}
}

void CAdminWorker::OutputNotify() {
	if (m_sender->Send() < 0) {
		log_error("send error!");
		delete this;
		return;
	}

	m_sender->SendDone() ? DisableOutput() : EnableOutput();
	DelayApplyEvents();
}

void CAdminWorker::ProcessRequest() {
	switch (m_header.cmd) {
	case AC_AddModule: {
		ttc::agent::AddModuleRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}

		ProcessAddModule(req.module(), req.addr().c_str(),
				req.accesstoken().c_str());
		break;
	}

	case AC_RemoveModule: {
		ttc::agent::RemoveModuleRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}
		ProcessRemoveModule(req.module());
		break;
	}

	case AC_AddCacheServer: {
		ttc::agent::AddCacheServerRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}
		ProcessAddCacheServer(req.module(), req.instanceid(),
				req.name().c_str(), req.addr().c_str(), req.virtual_node(),
				req.hotbak_addr().c_str(), req.mode());
		break;
	}

	case AC_RemoveCacheServer: {
		ttc::agent::RemoveCacheServerRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}
		ProcessRemoveCacheServer(req.module(), req.name().c_str(),
				req.virtual_node());
		break;
	}

	case AC_ReplaceCacheServer: {
		ttc::agent::ChangeCacheServerAddrRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}
		ProcessChangeCacheServerAddress(req.module(), req.name().c_str(),
				req.addr().c_str(), req.hotbak_addr().c_str(), req.mode());
		break;
	}

	case AC_ReloadConfig: {
		ttc::agent::ReloadConfigRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}

		ProcessReloadConfig(req.config().c_str());
		break;
	}
	case AC_Ping: {
		ProcessPing(m_threadgroup, m_gFdDispacher);
		break;
	}
	case AC_DumpConfig: {
		log_debug("enter AC_DumpConfig");
		std::string message = "";
		m_moduleMgr->DumpAllModuleConfig(message);
		log_debug("%s", message.c_str());
		ReplyWithMessage(AdminOk, message.c_str());
		break;
	}
	case AC_SwitchMechine: {
		ttc::agent::SwitchHotbakRequest req;
		if (!req.ParseFromArray(&m_request[0], m_request.size())) {
			log_error("invalid admin request received, pb parse error");
			delete this;
			return;
		}
		log_debug("size:%d", req.data_size());
		if(req.mode()>1 || req.mode()<0)
		{
			ReplyWithMessage(InvalidArg, "error mode received");
			break;
		}
		for (int i = 0; i < req.data_size(); i++) {
			log_debug("busid:%d,name:%s,mode:%d", req.data(i).businessid(),req.data(i).name().c_str(), req.mode());
			ProcessSwitchCacheServer(req.data(i).businessid(),req.data(i).name(), req.mode());
		}
		ReplyOk();
		break;
	}
	default:
		log_error("invalid admin cmd received: %d", m_header.cmd);
		ReplyWithMessage(CmdUnkown, "invalid cmd code");
		break;
	}
}

void CAdminWorker::ProcessAddModule(uint32_t module, const char *listenAddr,
		const char *accessToken) {
	if (m_moduleMgr->FindModule(module)) {
		ReplyWithMessage(ModuleAlreadyExist, "module already exist");
		return;
	}

	CStatManager *manager = CreateStatManager(module);
	if (NULL == manager) {
		ReplyWithMessage(AdminFail, "create StatManager error");
		return;
	}
	CStatManagerContainerThread::getInstance()->AddStatManager(module, manager);
	CModule *m = new CModule(module, m_ownerThread, m_threadgroup, m_gConfigObj,
			manager, m_gFdDispacher);

	if (!m) {
		ReplyWithMessage(AdminFail, "create module faield");
		CStatManagerContainerThread::getInstance()->DeleteStatManager(module);
		delete (manager);
		return;
	}

	if (m->Listen(listenAddr) != 0) {
		delete m;
		ReplyWithMessage(AdminFail, "listen failed");
		CStatManagerContainerThread::getInstance()->DeleteStatManager(module);
		delete (manager);
		return;
	}

	if (accessToken) {
		m->SetAccessToken(accessToken);
	}

	m_moduleMgr->AddModule(m);

	ConfigAddModule(module, listenAddr, accessToken);

	ReplyOk();
}

void CAdminWorker::ProcessRemoveModule(uint32_t module) {
	CModule *m = m_moduleMgr->FindModule(module);
	if (!m) {
		ReplyWithMessage(ModuleNotFound, "module not found");
		return;
	}

	m_moduleMgr->DeleteModule(module);
	CStatManagerContainerThread::getInstance()->DeleteStatManager(module);
	ConfigRemoveModule(module);
	ReplyOk();
}

void CAdminWorker::ProcessAddCacheServer(uint32_t module, uint32_t id,
		const char *name, const char *addr, int virtualNode,
		const char *hotbak_addr, int mode) {
	CModule *m = m_moduleMgr->FindModule(module);
	if (!m) {
		ReplyWithMessage(ModuleNotFound, "module not found");
		return;
	}
	if(mode>1 || mode <0)
	{
		ReplyWithMessage(InvalidArg, "error mode received");
		return;
	}

	if (m->DispachAddCacheServer(name, addr, virtualNode, hotbak_addr, mode)
			!= 0) {
		ReplyWithMessage(AdminFail, "add cache server failed");
		return;
	}
	std::string strmode;
	if (mode == 0) {
		strmode = "master";
	} else {
		strmode = "slave";
	}
	ConfigAddCacheServer(module, id, name, addr, hotbak_addr,strmode.c_str());
	ReplyOk();
}

void CAdminWorker::ProcessRemoveCacheServer(uint32_t module, const char *name,
		int virtualNode) {
	CModule *m = m_moduleMgr->FindModule(module);
	if (!m) {
		ReplyWithMessage(ModuleNotFound, "module not found");
		return;
	}

	if (m->DispachRemoveCacheServer(name, virtualNode) != 0) {
		ReplyWithMessage(AdminFail, "remove cache server failed");
		return;
	}

	ConfigRemoveCacheServer(module, name);
	ReplyOk();
}

/*
 */
void CAdminWorker::ProcessChangeCacheServerAddress(uint32_t module,
		const char *name, const char *addr, const char *hotbak_addr, int mode) {
	CModule *m = m_moduleMgr->FindModule(module);
	if (!m) {
		ReplyWithMessage(ModuleNotFound, "module not found");
		return;
	}

	if (m->DispachRemoveCacheServer(name, ADMIN_ALL_POINTERS) != 0) {
		ReplyWithMessage(AdminFail, "cache server not found");
		return;
	}
	if(mode>1 || mode <0)
	{
		ReplyWithMessage(InvalidArg, "error mode receive");
		return;
	}

	m->DispachAddCacheServer(name, addr, ADMIN_ALL_POINTERS, hotbak_addr,mode);
	std::string strmode;
	if (mode == 0) {
		strmode = "master";
	}
	else if(mode == 1)
	{
		strmode = "slave";
	}
	ConfigChangeCacheServerAddr(module, name, addr, hotbak_addr,
			strmode.c_str());
	ReplyOk();
}

void CAdminWorker::ReplyWithMessage(uint8_t code, const char *msg) {
	ttc::agent::Reply reply;
	reply.set_status(code);
	reply.set_msg(msg);

	int size = sizeof(ProtocolHeader) + reply.ByteSize();
	char *buf = (char *) malloc(size);

	ProtocolHeader *header = (ProtocolHeader *) buf;
	header->cmd = AC_Reply;
	header->magic = ADMIN_PROTOCOL_MAGIC;
	header->length = size - sizeof(ProtocolHeader);
	reply.SerializeWithCachedSizesToArray(
			(unsigned char *) buf + sizeof(ProtocolHeader));
	m_sender->AddPacket(buf, size);
	OutputNotify();
}

void CAdminWorker::ReplyOk() {
	ReplyWithMessage(AdminOk, "succeed.");
}

void CAdminWorker::ProcessReloadConfig(const char *config) {
	std::string errorMessage;

	if (!IsConfigValid(config, &errorMessage)) {
		ReplyWithMessage(InvalidConfigFile, errorMessage.c_str());
		log_error("invalid config received: %s", errorMessage.c_str());
		log_error("config : %s", config);
		return;
	}

	ReplyWithMessage(AdminOk, "config accepted. restart now ...");

	WriteConfig(config);
}

void CAdminWorker::ProcessPing(CPollThreadGroup *threadgroup,
		CFdDispacher *gFdDispacher) {
	BaseTask *btask = new BaseTask();
	btask->SetCmd(AGENTPING);
	btask->SetSyncFlag(1);
	for (int i = 0; i < threadgroup->GetPollThreadSize(); i++) {
		gFdDispacher[i].Push(btask);
	}
	ReplyWithMessage(HostExist, "agent exist!");
	delete (btask);
}

void CAdminWorker::ProcessSwitchCacheServer(uint32_t module, std::string name,
		uint32_t mode) {
	std::string modeName;
	CModule *m = m_moduleMgr->FindModule(module);
	if (!m) {
		//ReplyWithMessage(ModuleNotFound, "module not found");
		return;
	}

	if (m->DispachSwitchCacheServer(name, mode) != 0) {
		ReplyWithMessage(AdminFail, "add cache server failed");
		return;
	}
	if (mode == 0) {
		modeName = "master";
	} else {
		modeName = "slave";
	}
	ConfigSwitchCacheServerAddr(module, name.c_str(), modeName.c_str());
	//ReplyOk();
}
