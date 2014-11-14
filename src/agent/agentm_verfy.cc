#include<string.h>
#include<stdio.h>
#include<tinyxml.h>
#include<curl_http.h>
#include "log.h"
#include "sockaddr.h"
#include "sock_util.h"
#include <errno.h>
#include"admin.h"
#include "admin_protocol.pb.h"
#include<curl_http.h>
#include"json/json.h"
#include "config.h"
char gConfig[256] = "../conf/agent.xml";
#define AGENT_CONF_NAME  "../conf/agent.conf"
char agentFile[256] = AGENT_CONF_NAME;
CConfig* gConfigObj = new CConfig;
std::string url = "";
const int HttpServiceTimeOut=3;
int AgentMVerfy(int agentid, std::string agentmAddress);
int Report(std::string id, std::string s_id,std::string type);
int main(int argc, char** argv) {
	if (argc != 2) {
		printf("usage: %s manual\n", argv[0]);
		return -1;
	}
	if (gConfigObj->ParseConfig(agentFile, "monitor", true)) {
		return -1;
	}
	url=gConfigObj->GetStrVal("monitor", "AgentMonitorReportURL");
	_init_log_("dtc_agentmonitor", "../log");
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
	int logLevel=atoi(agentconf->Attribute("LogLevel"));
	_set_log_level_(logLevel);
	const char* agentid = agentconf->Attribute("AgentId");
	if (!agentid) {
		log_error("agentid conf miss");
		return -1;
	}
	std::string strAgentId=agentid;
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
	int ret = AgentMVerfy(gAgentId, gMasterAddr);
	int count = 0;
	while(ret==-1 && count<2)
	{
		ret = AgentMVerfy(gAgentId, gMasterAddr);
		count++;
	}
	if (-1 == ret) {
		log_info("The agentm is shutdown! but not report to monitor centor");
		//Report(strAgentId,"4","1");
	}
	if (-2 == ret) {
		log_info("The agent is shutdown! report to Monitor center");
		Report(strAgentId,"1","1");
	}
	return 0;
}

int AgentMVerfy(int agentid, std::string agentmAddress) {
	log_info("current master addr: %s", agentmAddress.c_str());
	CSocketAddress agentmaddr;
	const char *err = agentmaddr.SetAddress(agentmAddress.c_str(),
			(const char *) NULL);
	if (err) {
		log_error("invalid master addr!");
		return -1;
	}
	int netfd = agentmaddr.CreateSocket();
	if (netfd < 0) {
		log_error("create socket error: %d, %m", errno);
		return -1;
	}
	struct timeval tv = { 0, 50000 }; //50ms
	setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (agentmaddr.ConnectSocket(netfd) < 0) {
		log_error("connect master error: %d, %m", errno);
		close(netfd);
		netfd = -1;
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
	header->cmd = AC_QueryAgentStatus;
	queryRequest.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));
	if (send_n(netfd, (char *) buf, byteSize + sizeof(ProtocolHeader)) < 0) {
		log_error("send QueryAgentStatus error: %m");
		close(netfd);
		free(buf);
		return -1;
	}
	if (recv_n(netfd, (char *) buf, sizeof(ProtocolHeader)) < 0) {
		log_error("recv QueryAgentStatus header error: %m");
		close(netfd);
		free(buf);
		return -1;
	}
	if (!IsProtocolHeaderValid(header)
			|| header->cmd != AC_QueryAgentStatusReply) {
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
	ttc::agent::QueryAgentStatusReply reply;
	if (!reply.ParseFromArray(buf + sizeof(ProtocolHeader), header->length)) {
		log_error("parse QueryAgentStatus_reply body error");
		free(buf);
		close(netfd);
		return -1;
	}
	free(buf);
	close(netfd);
	int id = reply.id();
	int dbtime = reply.dbtime();
	int lmtime = reply.lmtime();
	if (dbtime - lmtime > 10) {
		log_info("The agent is %d shutdown! report to Monitor center", id);
		return -2;
	}
	return 0;
}

int Report(std::string id, std::string s_id,std::string type) {
	BuffV buf;
	Json::Reader reader;
	Json::Value root;
	std::string strResp;
	int ret;
	std::string reportUrl=url+"?id="+id+"&strategy_id="+s_id+"&type="+type;
	CurlHttp curlHttp;
	curlHttp.SetTimeout(HttpServiceTimeOut);
	ret=curlHttp.HttpRequest(reportUrl, &buf, false);
	strResp = buf.Ptr();
	log_info("strResp:%s", strResp.c_str());
	if (ret != 0) {
		log_error("id:%s strategy_id:%s:report http error!~~~~~~", id.c_str(),s_id.c_str());
		return -1;
	}
	strResp = buf.Ptr();
	if (!reader.parse(strResp.c_str(), root, false)) {
		log_error("id:%s strategy_id:%s :parse http return message error!~~~~~~",id.c_str(),s_id.c_str());
		return -1;
	}
	std::string result = root["status"].asString();
	if ("success" != result) {
		log_error("id:%s strategy_id:%s:report return error,report failed!~~~~~~",id.c_str(),s_id.c_str());
		return -1;
	}
	return 0;
}

