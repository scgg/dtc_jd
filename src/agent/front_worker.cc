/*
 * =====================================================================================
 *
 *       Filename:  worker.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2010 05:47:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "front_worker.h"
#include "poll_thread.h"
#include "module.h"
#include "sender.h"
#include "receiver.h"
#include "decode.h"
#include "cache_error.h"
#include "stat_definition.h"
#include "stat_agent.h"
#include <vector>

CFrontWorker::CFrontWorker(uint64_t id, int fd, CPollThread *thread,
		CModule *module,CStatManager *manager) :
		m_id(id), m_ownerModule(module),m_statManager(manager) {
	netfd = fd;
	m_sender = new CAGSender(fd, 16);
	m_sender->Build();
	m_receiver = new CAGReceiver(fd);
	m_ownerThread = thread;
	EnableInput();
	m_moduleId = module->Id();
	timerList = thread->GetTimerList(5);

	if (timerList == NULL)
		log_error("timer list is null");
	AttachTimer(timerList);
	AttachPoller(thread);
	m_statItem=new CStatItem[100];
	m_statItem[FRONT_CONN_COUNT]=m_statManager->GetItem(FRONT_CONN_COUNT);
	m_statItem[FRONT_ACCEPT_COUNT]=m_statManager->GetItem(FRONT_ACCEPT_COUNT);
	m_statItem[REQUEST_ELAPES]=m_statManager->GetItem(REQUEST_ELAPES);
	m_statItem[REQUESTS]=m_statManager->GetItem(REQUESTS);
	m_statItem[REQPKGSIZE]=m_statManager->GetItem(REQPKGSIZE);
	m_statItem[PKG_ENTER_COUNT]=m_statManager->GetItem(PKG_ENTER_COUNT);
	m_statItem[SVR_ADMIN_COUNT]=m_statManager->GetItem(SVR_ADMIN_COUNT);
	m_statItem[GET_COUNT]=m_statManager->GetItem(GET_COUNT);
	m_statItem[PURGE_COUNT]=m_statManager->GetItem(PURGE_COUNT);
	m_statItem[INSERT_COUNT]=m_statManager->GetItem(INSERT_COUNT);
	m_statItem[UPDATE_COUNT]=m_statManager->GetItem(UPDATE_COUNT);
	m_statItem[DELETE_COUNT]=m_statManager->GetItem(DELETE_COUNT);
	m_statItem[OTHERS_COUNT]=m_statManager->GetItem(OTHERS_COUNT);
	m_statItem[RSPPKGSIZE]=m_statManager->GetItem(RSPPKGSIZE);
	m_statItem[PKG_LEAVE_COUNT]=m_statManager->GetItem(PKG_LEAVE_COUNT);
	m_statItem[GET_HIT]=m_statManager->GetItem(GET_HIT);
	m_statItem[RESPONSES]=m_statManager->GetItem(RESPONSES);
	m_statItem[FRONT_CONN_COUNT]++;
	m_statItem[FRONT_ACCEPT_COUNT]++;
}

CFrontWorker::~CFrontWorker() {
	log_debug("id :%llu frontworker has been free!", m_id);
	delete m_sender;
	delete m_receiver;
	std::map<uint64_t, uint64_t>::iterator iter = m_pkg2Elapse.begin();
	for (; iter != m_pkg2Elapse.end(); ++iter)
	{
		int elapse = GET_TIMESTAMP() - iter->second;
		m_statItem[REQUEST_ELAPES]+= elapse;
		m_statItem[REQUESTS]++;
	}
	m_statItem[FRONT_CONN_COUNT]--;
	delete []m_statItem;
	m_pkg2Elapse.clear();
}

int CFrontWorker::Send() {
	if (m_sender->Send() < 0)
		return -1;

	m_sender->SendDone() ? DisableOutput() : EnableOutput();
	DelayApplyEvents();

	return 0;
}

void CFrontWorker::OutputNotify() {
	if (Send() < 0) {
		log_error("send error!");
		EXIT();
	}
}

void CFrontWorker::TimerNotify() {
	log_debug("enter");
	AttachTimer(timerList);

	std::map<uint64_t, uint64_t>::iterator iter = m_pkg2Elapse.begin();

	for (; iter != m_pkg2Elapse.end();) {
		int elapse = GET_TIMESTAMP() - iter->second;

		if (elapse > 10000000)
		{
			m_pkg2Elapse.erase(iter++);
			log_info("req found elapse more than 10 seconds");
			m_statItem[REQUEST_ELAPES]+= elapse;
			m_statItem[REQUESTS]++;
		}
		else {
			++iter;
		}

	}
}

int CFrontWorker::Recv() {
	while (true) {
		int rtn = m_receiver->Recv();
		if (rtn < 0)
			return -1;
		else if (rtn == 0)
			return 0;

		char *buffer = NULL;
		uint32_t len;
		m_receiver->GetPacket(&buffer, &len);

		m_statItem[REQPKGSIZE]+=len;
		m_statItem[PKG_ENTER_COUNT]++;
		CPacketHeader *header = (CPacketHeader *) buffer;
		if (header->cmd == DRequest::SvrAdmin) {
			m_statItem[SVR_ADMIN_COUNT]++;
			log_error("got a SvrAdmin request");
			ReplyWithError("SvrAdmin not support", buffer);
			free(buffer);
			continue;
		}

		char *packedKey = NULL;
		uint32_t packedKeyLen;
		std::string accessToken = "";
		std::string verifyToken = m_ownerModule->GetAccessToken();
		bool idSetMark = false;
		uint64_t agentId;
		rtn = ExtractPackedKey(buffer, len, &packedKey, &packedKeyLen,
				accessToken, idSetMark, agentId);

		if (rtn < 0) {
			log_error("ExtractPackedKey error");
			ReplyWithError("ExtractPackedKey error", buffer);
			free(buffer);
			continue;
		}
		log_info("cmd %d flags:%d accesstoken:%s verifytoken:%s", header->cmd,
				header->flags, accessToken.c_str(), verifyToken.c_str());
		//linjinming verify accesstoken[2014-06-04 18:37]
		if (verifyToken != "" && verifyToken != accessToken) {
			log_error("verify token error");
			ReplyWithError("verify token error", buffer);
			free(buffer);
			continue;
		}

		char *newPacket = NULL;
		uint32_t newPacketLen;
		uint64_t pkgSeq = 0;
		if (idSetMark == false) {
			rtn = EncodeAgentIdToNewRequest(m_id, buffer, len, &newPacket,
					&newPacketLen, pkgSeq);
		} else {
			log_debug("agent id has been set:%llu", agentId);
			rtn = EncodeAgentIdToNewRequest((agentId << 32) | m_id, buffer, len,
					&newPacket, &newPacketLen, pkgSeq);
		}
		if (rtn < 0) {
			log_error("EncodeAgentIdToNewRequest error");
			free(newPacket);
			ReplyWithError("EncodeAgentIdToNewRequest error", buffer);
			free(buffer);
			free(packedKey);
			continue;
		}
		log_debug("request coming");
		log_debug("the request frontworker id is %llu", m_id);
		rtn = m_ownerModule->DispatchRequest(packedKey, packedKeyLen, newPacket,
				newPacketLen, m_ownerThread);
		if (rtn < 0) {
			log_error("Error in DispatchRequest");
			free(newPacket);
			ReplyWithError("DispatchRequest Error", buffer);
			free(buffer);
			free(packedKey);
			continue;
		}

		m_pkg2Elapse[pkgSeq] = GET_TIMESTAMP();
		free(buffer);
		free(packedKey);
	}

	return 0;
}

void CFrontWorker::InputNotify() {
	log_debug("input event notify");
	if (Recv() < 0) {
		EXIT();
	}
}

void CFrontWorker::HangupNotify(void) {
	EXIT();
}

int CFrontWorker::ExtractPackedKey(char *buf, uint32_t len, char **packedKey,
		uint32_t *packedKeyLen, std::string &accessToken, bool &mark,
		uint64_t &agentId) {
	CPacketHeader *header = (CPacketHeader *) buf;
	if (header->cmd == DRequest::Nop) {
		(*CStatManagerContainerThread::getInstance())(m_moduleId, NOP_COUNT)++;static
		uint64_t randomHashSeed = 1;
		*packedKey = (char *) malloc(sizeof(uint64_t));
		*packedKeyLen = sizeof(uint64_t);
		*(uint64_t *) (*packedKey) = randomHashSeed++;

		CVersionInfo vi;
		int rtn = DecodeSimpleSection(buf + sizeof(CPacketHeader),
				header->len[DRequest::Section::VersionInfo], vi, DField::None);
		if (rtn != 0) {
			log_error("decode version info error, %d", rtn);
			return -1;
		}

		if (vi.TagPresent(18)) {
			CBinary ak = vi.AccessKey();

			if (ak.len > 0) {
				std::vector<char> utf8(ak.len + 1);
				memset(&utf8[0], 0, ak.len + 1);

				memcpy(&utf8[0], ak.ptr, ak.len);

				accessToken = &utf8[0];
			}
		}

		if (vi.TagPresent(17)) {
			mark = true;
			agentId = vi.GetAgentClientId();
		}

		return 0;
	}

	switch (header->cmd) {
		case DRequest::Get:
			m_statItem[GET_COUNT]++;
			break;
		case DRequest::Purge:
			m_statItem[PURGE_COUNT]++;
			break;
		case DRequest::Insert:
			m_statItem[INSERT_COUNT]++;
			break;
		case DRequest::Update:
			m_statItem[UPDATE_COUNT]++;
			break;
		case DRequest::Delete:
			m_statItem[DELETE_COUNT]++;
			break;
		default:
			m_statItem[OTHERS_COUNT]++;
			break;
	}

	char *readPos = buf + sizeof(CPacketHeader);

	//version info 0
	CVersionInfo vi;
	uint32_t viLen = header->len[DRequest::Section::VersionInfo];
	int rtn = DecodeSimpleSection(readPos, viLen, vi, DField::None);
	if (rtn != 0) {
		log_error("decode version info error, %d", rtn);
		return -1;
	}

	int keyType = vi.KeyType();

	readPos += header->len[DRequest::Section::VersionInfo];

	//table def 1
	readPos += header->len[DRequest::Section::TableDefinition];

	//request info 2
	CRequestInfo qi;
	rtn = DecodeSimpleSection(readPos,
			header->len[DRequest::Section::RequestInfo], qi, keyType);
	if (rtn != 0) {
		log_error("decode request info error, %d", rtn);
		return -1;
	}

	if (header->flags & DRequest::Flag::MultiKeyValue) {
		log_error("batch get not supported");
		return -1;
	}

	if (vi.GetTag(10) && vi.GetTag(10)->u64 > 1) {
		log_error("multi key value not supported yet");
		return -1;
	}

	if (vi.TagPresent(18)) {
		CBinary ak = vi.AccessKey();

		if (ak.len > 0) {
			std::vector<char> utf8(ak.len + 1);
			memset(&utf8[0], 0, ak.len + 1);

			memcpy(&utf8[0], ak.ptr, ak.len);

			accessToken = &utf8[0];
		}
	}
	if (vi.TagPresent(17)) {
		mark = true;
		agentId = vi.GetAgentClientId();
	}

	if ((keyType == DField::Signed || keyType == DField::Unsigned)
			&& header->cmd == DRequest::Insert && !qi.Key()) {
		log_debug("insert request without key, assume auto-increment key");
		*packedKey = (char *) malloc(sizeof(uint64_t));
		*packedKeyLen = sizeof(uint64_t);
		*(uint64_t *) (*packedKey) = 1;
		return 0;
	}

	switch (keyType) {
	case DField::Signed:
	case DField::Unsigned:
		*packedKey = (char *) malloc(sizeof(uint64_t));
		*packedKeyLen = sizeof(uint64_t);
		*(uint64_t *) (*packedKey) = qi.Key()->u64;
		return 0;
	case DField::String:
		*packedKeyLen = qi.Key()->str.len + 1;
		*packedKey = (char *) malloc(qi.Key()->str.len + 1);
		*(*packedKey) = qi.Key()->str.len;
		for (int i = 0; i < qi.Key()->str.len; ++i)
			(*packedKey)[i + 1] = INTERNAL_TO_LOWER(qi.Key()->str.ptr[i]);
		return 0;
	case DField::Binary:
		*packedKeyLen = qi.Key()->str.len + 1;
		*packedKey = (char *) malloc(qi.Key()->str.len + 1);
		*(*packedKey) = qi.Key()->str.len;
		memcpy(*packedKey + 1, qi.Key()->str.ptr, qi.Key()->str.len);
		return 0;

	default:
	case DField::Float:
		log_error("wrong key type, %d", keyType);
		return -1;
	}
}

int GetBodyLen(const CPacketHeader *header) {
	int len = 0;
	for (int i = 0; i < DRequest::Section::Total; ++i) {
		len += header->len[i];
	}

	return len;
}

int CFrontWorker::EncodeAgentIdToNewRequest(uint64_t id, char *buf,
		uint32_t len, char **newPacket, uint32_t *newPacketLen,
		uint64_t &pkgSeq) {
	CPacketHeader *header = (CPacketHeader *) buf;
	uint32_t bodyLen = GetBodyLen(header);

	//version info 0
	CVersionInfo vi;
	uint32_t viLen = header->len[DRequest::Section::VersionInfo];
	char *readPos = buf + sizeof(CPacketHeader);
	int rtn = DecodeSimpleSection(readPos, viLen, vi, DField::None);
	if (rtn != 0) {
		log_error("decode version info error, %d", rtn);
		return -1;
	}

	vi.SetAgentClientId(id);
	pkgSeq = vi.SerialNr();
	uint32_t newViLen = EncodedBytesSimpleSection(vi, DField::None);
	uint32_t newBodyLen = bodyLen - viLen + newViLen;
	*newPacketLen = sizeof(CPacketHeader) + newBodyLen;
	*newPacket = (char *) malloc(*newPacketLen);

	char *writePos = *newPacket;
	memcpy(writePos, buf, sizeof(CPacketHeader));
	((CPacketHeader *) writePos)->len[DRequest::Section::VersionInfo] =
			newViLen;
	writePos += sizeof(CPacketHeader);
	writePos = EncodeSimpleSection(writePos, vi, DField::None);
	memcpy(writePos, buf + sizeof(CPacketHeader) + viLen, bodyLen - viLen);

	return 0;
}

void CFrontWorker::ReplyWithError(const char *errMsg, char *buf) {
	CResultInfo ri;
	ri.SetTotalRows(0);
	ri.SetError(-EC_SERVER_ERROR, "Agent::FrontWorker", errMsg);
	ri.SetTimestamp(time(NULL));

	CPacketHeader *header = (CPacketHeader *) buf;
	//version info 0
	CVersionInfo vi;
	char *readPos = buf + sizeof(CPacketHeader);
	uint32_t viLen = header->len[DRequest::Section::VersionInfo];
	int rtn = DecodeSimpleSection(readPos, viLen, vi, DField::None);
	if (rtn != 0) {
		log_error("decode version info error, %d", rtn);
		return;
	}

	uint32_t totalLen = sizeof(CPacketHeader)
			+ EncodedBytesSimpleSection(vi, DField::None)
			+ EncodedBytesSimpleSection(ri, vi.KeyType());

	char *packet = (char *) malloc(totalLen);
	CPacketHeader *newHeader = (CPacketHeader *) packet;
	newHeader->version = 1;
	newHeader->scts = 8;
	newHeader->flags = DRequest::Flag::KeepAlive
			| (header->flags & DRequest::Flag::MultiKeyValue);
	newHeader->cmd = DRequest::ResultCode;

	newHeader->len[DRequest::Section::VersionInfo] = EncodedBytesSimpleSection(
			vi, DField::None);
	newHeader->len[DRequest::Section::TableDefinition] = 0;
	newHeader->len[DRequest::Section::RequestInfo] = 0;
	newHeader->len[DRequest::Section::ResultInfo] = EncodedBytesSimpleSection(
			ri, vi.KeyType());
	newHeader->len[DRequest::Section::UpdateInfo] = 0;
	newHeader->len[DRequest::Section::ConditionInfo] = 0;
	newHeader->len[DRequest::Section::FieldSet] = 0;
	newHeader->len[DRequest::Section::ResultSet] = 0;

	char *writePos = packet + sizeof(CPacketHeader);
	writePos = EncodeSimpleSection(writePos, vi, DField::None);
	writePos = EncodeSimpleSection(writePos, ri, vi.KeyType());

	m_statItem[RSPPKGSIZE]+=totalLen;
	m_statItem[PKG_LEAVE_COUNT]++;
	m_sender->AddPacket(packet, totalLen);
	Send();
}

int CFrontWorker::SendResponse(char *buf, uint32_t len, CHitInfo &hitInfo,
		uint64_t pkgSeq) {
	log_info("SendReponse enter");

	std::map<uint64_t, uint64_t>::iterator iter = m_pkg2Elapse.find(pkgSeq);

	if (hitInfo.isGet() && hitInfo.isHit()) {
		m_statItem[GET_HIT]++;
	}

	if (iter != m_pkg2Elapse.end()) {
		int elapse = GET_TIMESTAMP() - m_pkg2Elapse[pkgSeq];
		m_pkg2Elapse.erase(iter);
		m_statItem[REQUEST_ELAPES]+= elapse;
		m_statItem[REQUESTS]++;
		m_statItem[RESPONSES]++;
		log_info("response seq:%llu elapse %d", pkgSeq, elapse);
	} else {
		log_info("could not find response seq:%llu", pkgSeq);
	}
	m_statItem[RSPPKGSIZE] +=len;
	m_statItem[PKG_LEAVE_COUNT]++;
	if(m_sender->AddPacket(buf, len) < 0)
	{
		return -1;
	}
	return Send();

}

void CFrontWorker::EXIT()
{
	m_ownerModule->ReleseFrontWorker(this->m_ownerThread,this->m_id);
}

