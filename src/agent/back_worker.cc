/*
 * =====================================================================================
 *
 *       Filename:  back_worker.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2010 06:53:19 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "back_worker.h"
#include "log.h"
#include "poll_thread.h"
#include "receiver.h"
#include "sender.h"
#include "constant.h"
#include "protocol.h"
#include "decode.h"
#include "module.h"
#include "hitinfo.h"
//#include <netinet/tcp.h>

CBackWorker::~CBackWorker() {
	delete m_sender;
	delete m_receiver;
}

int CBackWorker::ConnectTTC() {
	log_debug("CAGAgent::ConnectTTC start");

	if (m_status != HS_DOWN) {
		log_error("try connect ttc when agent status Up");
		return -1;
	}

	if (netfd > 0)
		close(netfd);

	netfd = ttcAddr.CreateSocket();
	if (netfd < 0) {
		log_error("TTCAgent create socket failed: %m, %d", errno);
		return -1;
	}

	//int optval = 1;
	//setsockopt (netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof (optval));

	m_status = HS_CONNECTING;

	/* blocking connect */
	if (ttcAddr.ConnectSocket(netfd) < 0) {
		log_error("TTCAgent connect ttc failed: %m, %d", errno);
		close(netfd);
		netfd = -1;
		m_status = HS_DOWN;
		return -1;
	}

	EnableInput();
	DisableOutput();

	if (AttachPoller(m_ownerThread) < 0) {
		log_error("TTCAgent attach poller failed");
		close(netfd);
		netfd = -1;
		m_status = HS_DOWN;
		return -1;
	}

	m_receiver = new CAGReceiver(netfd);
	if (NULL == m_receiver) {
		log_crit("TTCAgent no mem create receiver");
		close(netfd);
		netfd = -1;
		m_status = HS_DOWN;
		return -1;
	}

	m_sender = new CAGSender(netfd, 4096 * 4);
	if (NULL == m_sender) {
		delete m_receiver;
		m_receiver = NULL;
		close(netfd);
		netfd = -1;
		m_status = HS_DOWN;
		log_crit("TTCAgent no mem new agent m_sender");
		return -1;
	}

	if (m_sender->Build() < 0) {
		delete m_receiver;
		m_receiver = NULL;
		delete m_sender;
		m_sender = NULL;
		close(netfd);
		netfd = -1;
		m_status = HS_DOWN;
		log_crit("TTCAgent no mem init agent m_sender");
		return -1;
	}

	m_status = HS_UP;
	log_debug("CAGAgent::ConnectTTC stop,m_status=%d", m_status);

	DelayApplyEvents();

	return 0;
}

void CBackWorker::Exit() {
	DetachPoller();
	ownerUnit = NULL;
	DisableTimer();
	close(netfd);
	netfd = -1;
	delete m_receiver;
	m_receiver = NULL;
	delete m_sender;
	m_sender = NULL;
	m_status = HS_DOWN;
}

/* backworker interface */
int CBackWorker::Build(const std::string & addrstr, CPollThread *thread,
		CModule *module) {
	m_ownerThread = thread;
	m_ownerModule = module;
	const char *err = ttcAddr.SetAddress(addrstr.c_str(), (const char *) NULL);
	if (err) {
		log_error("bad ttc address");
		return -1;
	}

	m_status = HS_DOWN;
	return 0;
}

bool CBackWorker::GetReady() {
	if (m_status == HS_UP) {
		log_debug("the status is HS_UP not connect again");
		return true;
	}
	if (ConnectTTC() < 0)
		return false;

	return true;
}

int CBackWorker::HandleRequest(const char *buf, int len) {
	if (m_sender->AddPacket(buf, len) < 0) {
		log_error("fatal error, m_sender can not be used any more");
		Exit();
		return -1;
	}
	Send();
	//AttachReadyTimer(m_ownerThread);
	return 0;
}

/* send */
int CBackWorker::Send() {
	if (m_sender->Send() < 0) {
		log_error("m_sender send error");
		return -1;
	}

	if (m_sender->SendDone())
		DisableOutput();
	else
		EnableOutput();

	DelayApplyEvents();
	return 0;
}

void CBackWorker::OutputNotify() {
	if (m_status == HS_DOWN)
		return;

	if (Send() < 0) {
		log_error("back worker send failed");
		Exit();
	}
}

void CBackWorker::TimerNotify() {
	OutputNotify();
}

int GetBodyLen2(const CPacketHeader *header) {
	int len = 0;
	for (int i = 0; i < DRequest::Section::Total; ++i) {
		len += header->len[i];
	}

	return len;
}

int CBackWorker::Recv() {
	while (true) {
		int rtn = m_receiver->Recv();
		if (rtn < 0)
			return -1;
		else if (rtn == 0)
			return 0;

		char *buffer = NULL;
		uint32_t len;

		//got a packet
		m_receiver->GetPacket(&buffer, &len);
		CVersionInfo vi;
		CPacketHeader *header = (CPacketHeader *) buffer;
		uint32_t viLen = header->len[DRequest::Section::VersionInfo];
		int err = DecodeSimpleSection(buffer + sizeof(CPacketHeader), viLen, vi,
				DField::None);
		if (err != 0) {
			log_error("decode version info error, %d", err);
			free(buffer);
			return -1;
		}
		//FIXME: 17 is for agent id
		if (!vi.TagPresent(17)) {
			free(buffer);
			log_error("tag 17 (agent id) not exist");

			return -1;
		}

		CHitInfo hitInfo;
		log_info("cmd:%d", header->cmd);
		int resLen = header->len[DRequest::Section::ResultInfo];
		if ((header->cmd == DRequest::ResultSet || header->cmd == DRequest::ResultCode) && resLen != 0) {
			CResultInfo ri;
			uint32_t requestInfoPos =
					header->len[DRequest::Section::VersionInfo]
							+ header->len[DRequest::Section::TableDefinition]
							+ header->len[DRequest::Section::RequestInfo];

			err = DecodeSimpleSection(
					buffer + sizeof(CPacketHeader) + requestInfoPos, resLen, ri,
					DField::None);

			if (err != 0) {
				log_error("decode result info error, %d", err);
				free(buffer);
				return -1;
			}
			//FIXME: 9 is for hit tag
			if (ri.TagPresent(9)) {
				log_info("hit tag is %d", ri.HitFlag());
				hitInfo.setHit(ri.HitFlag());
				hitInfo.setGet(true);
			}
		}

		uint64_t clientId = vi.GetAgentClientId() & 0xffffffff;
		uint64_t fclientId = (vi.GetAgentClientId()) >> 32;
		if (fclientId == 0) {
			//不需要每次都重新组装回包
			m_ownerModule->DispatchResponse(clientId, buffer, len, hitInfo,vi.SerialNr(), m_ownerThread);

		} else {
			uint32_t bodyLen = GetBodyLen2(header);
			log_debug("the lenth of body is %u", bodyLen);
			vi.SetAgentClientId(fclientId);
			uint32_t newViLen = EncodedBytesSimpleSection(vi, DField::None);
			uint32_t newBodyLen = bodyLen - viLen + newViLen;
			uint32_t newPacketLen = sizeof(CPacketHeader) + newBodyLen;
			char *newPacket = (char *) malloc(newPacketLen);
			char *writePos = newPacket;
			memcpy(writePos, buffer, sizeof(CPacketHeader));
			((CPacketHeader *) writePos)->len[DRequest::Section::VersionInfo] =
					newViLen;
			log_debug("the new line is %u", newViLen);
			writePos += sizeof(CPacketHeader);
			writePos = EncodeSimpleSection(writePos, vi, DField::None);
			memcpy(writePos, buffer + sizeof(CPacketHeader) + viLen,
					bodyLen - viLen);
			log_debug("the lenth of body is %u",
					GetBodyLen2((CPacketHeader * )newPacket));
			free(buffer);
			m_ownerModule->DispatchResponse(clientId, newPacket, newPacketLen,hitInfo, vi.SerialNr(), m_ownerThread);
		}
		// m_ownerModule->DispatchResponse(clientId, buffer, len, hitInfo, vi.SerialNr());
	}
}

void CBackWorker::InputNotify() {
	if (HS_DOWN == m_status)
		return;

	if (Recv() < 0) {
		log_error("Backworker recv error, exit");
		Exit();
	}
}

