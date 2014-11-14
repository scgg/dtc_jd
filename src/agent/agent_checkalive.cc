/*
 * agent_checkalive.cc
 *
 *  Created on: 2014Äê8ÔÂ27ÈÕ
 *      Author: Jiansong
 */

#include "agent_checkalive.h"
#include "stat_agent.h"
#include "checkalive_stat_definition.h"
#include "time.h"
#include <stdlib.h>

char gConfig[256] = "../conf/agent.xml";
#define AGENT_CONF_NAME  "../conf/agent.conf"
char agentFile[256] = AGENT_CONF_NAME;
CConfig* gConfigObj = new CConfig;
std::string url = "";
const int HttpServiceTimeOut = 3;
CStatManager *statM;

int AgentPing(int agentid, std::string agentAddress) {
	log_info("current master addr: %s", agentAddress.c_str());
	struct timeval start;
	struct timeval end;
	CSocketAddress agentaddr;
	const char *err = agentaddr.SetAddress(agentAddress.c_str(),
			(const char *) NULL);
	if (err) {
		log_error("invalid master addr!");
		return -1;
	}
	int netfd = agentaddr.CreateSocket();
	if (netfd < 0) {
		log_error("create socket error: %d, %m", errno);
		return -1;
	}
	struct timeval tv = { 0, 50000 }; //50ms
	setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (agentaddr.ConnectSocket(netfd) < 0) {
		log_error("connect master error: %d, %m", errno);
		close(netfd);
		return -1;
	}
	ttc::agent::QueryAgentStatus queryRequest;
	queryRequest.set_id(agentid);
	int byteSize = queryRequest.ByteSize();
	unsigned char *buf = (unsigned char *) malloc(
			byteSize + sizeof(ProtocolHeader));
	ProtocolHeader *header = (ProtocolHeader *) buf;
	header->magic = ADMIN_PROTOCOL_MAGIC;
	header->length = byteSize;
	header->cmd = AC_Ping;
	queryRequest.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));
	if (send_n(netfd, (char *) buf, byteSize + sizeof(ProtocolHeader)) < 0) {
		log_error("send QueryAgentStatus error: %m");
		close(netfd);
		free(buf);
		return -1;
	}
	gettimeofday(&start,NULL);
	if (recv_n(netfd, (char *) buf, sizeof(ProtocolHeader)) < 0) {
		log_error("recv QueryAgentStatus header error: %m");
		close(netfd);
		free(buf);
		return -1;
	}
	if (!IsProtocolHeaderValid(header) || header->cmd != AC_Reply) {
		log_error("invalid header recved: %x, %x, %x", header->magic,
				header->length, header->cmd);
		close(netfd);
		free(buf);
		return -1;
	}
	buf = (unsigned char *) realloc(buf,
			header->length + sizeof(ProtocolHeader));
	header = (ProtocolHeader *) buf;
	if (recv_n(netfd, (char *) buf + sizeof(ProtocolHeader), header->length)
			< 0) {
		log_error("recv QueryAgentStatus_reply body error: %m");
		free(buf);
		close(netfd);
		return -1;
	}
	gettimeofday(&end,NULL);
	double time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
	log_debug("time used is %f",time_use);
	ttc::agent::Reply reply;
	if (!reply.ParseFromArray(buf + sizeof(ProtocolHeader), header->length)) {
		log_error("parse QueryAgentStatus_reply body error");
		free(buf);
		close(netfd);
		return -1;
	}
	free(buf);
	close(netfd);
	log_info("agent is avalible:%s!", reply.msg().c_str());
	return 0;
}

int Report(std::string id, std::string s_id, std::string type) {
	BuffV buf;
	Json::Reader reader;
	Json::Value root;
	std::string strResp;
	int ret;
	std::string reportUrl = url + "?id=" + id + "&strategy_id=" + s_id
			+ "&type=" + type;
	CurlHttp curlHttp;
	curlHttp.SetTimeout(HttpServiceTimeOut);
	ret = curlHttp.HttpRequest(reportUrl, &buf, false);
	strResp = buf.Ptr();
	log_info("strResp:%s", strResp.c_str());
	if (ret != 0) {
		log_error("id:%s strategy_id:%s:report http error!~~~~~~", id.c_str(),
				s_id.c_str());
		return -1;
	}
	strResp = buf.Ptr();
	if (!reader.parse(strResp.c_str(), root, false)) {
		log_error(
				"id:%s strategy_id:%s :parse http return message error!~~~~~~",
				id.c_str(), s_id.c_str());
		return -1;
	}
	std::string result = root["status"].asString();
	if ("success" != result) {
		log_error(
				"id:%s strategy_id:%s:report return error,report failed!~~~~~~",
				id.c_str(), s_id.c_str());
		return -1;
	}
	return 0;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("usage: %s manual\n", argv[0]);
		return -1;
	}
	if (gConfigObj->ParseConfig(agentFile, "monitor", true)) {
		return -1;
	}
	time_t timep;
	statM = CreateCheckaliveStatManager();
	CStatItem totalPing = statM->GetItem(PING_TIMES_COUNT);
	CStatItem failPing = statM->GetItem(PING_FAIL_TIMES_COUNT);
	CStatItem contFailPing = statM->GetItem(PING_FAIL_TIMES_COUNT_CON);
	CStatItem pingStamp = statM->GetItem(PING_TIME_STAMP);
	url = gConfigObj->GetStrVal("monitor", "AgentMonitorReportURL");
	_init_log_("agent_checkalive", "../log");
	_set_log_level_(3);
	TiXmlDocument configdoc;
	if (!configdoc.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gConfig,
				configdoc.ErrorDesc(), configdoc.ErrorRow(),
				configdoc.ErrorCol());
		return -1;
	}
	TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *agentconf = rootnode->FirstChildElement("AGENT_CONFIG");
	if (!agentconf) {
		log_error("agent conf miss");
		return -1;
	}
	int logLevel = atoi(agentconf->Attribute("LogLevel"));
	_set_log_level_(logLevel);
	const char* agentid = agentconf->Attribute("AgentId");
	if (!agentid) {
		log_error("agentid conf miss");
		return -1;
	}
	std::string strAgentId = agentid;
	int gAgentId = atoi(agentid);
	const char* agentmaddr = agentconf->Attribute("MasterAddr");
	if (!agentmaddr) {
		log_error("MasterAddr conf miss");
		return -1;
	}
	std::string gMasterAddr = agentmaddr;

	const char* agentaddr = agentconf->Attribute("AdminListen");
	if (!agentaddr) {
		log_error("MasterAddr conf miss");
		return -1;
	}
	std::string gAgentAddr = agentaddr;

	time(&timep);
	long last_update = pingStamp;
	if ((timep - last_update) > 600) {
		log_error("time between is %d", (timep - last_update));
		pingStamp = timep;
		failPing = 0;
	}
	int ret = AgentPing(gAgentId, gAgentAddr);
	totalPing++;
	if (ret == -1) {
		Report(strAgentId, "2", "1");
		failPing++;
		contFailPing++;
		log_error("ping target host error! report to Monitor center");
	}
	else
	{
		contFailPing = 0;
	}
	if (contFailPing > 2) {//failPing > 10 ||
		log_debug("keepalived been acted");
		return 1;
	}
	return 0;
}

