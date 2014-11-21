/*
 * agentmonitor.cc
 *
 *  Created on: 2014年11月13日
 *      Author: Jiansong
 */
#include "agentmonitor.h"

#define MONITOR_CONF_NAME  "../conf/monitor.conf"
#define COMPENSITELIST_FILE "../stat/agentcompensitelist.dat"
#define AGENTSTATUS_FILE "../stat/agentstatus.dat"

char agentFile[256] = MONITOR_CONF_NAME;
CConfig* gConfigObj = new CConfig;

MYSQL* mysql_inst = new MYSQL;

std::string alarmurl = "";
std::string adminurl = "";

const char *host;
int port;
const char *username;
const char *password;
const char *db;
const char *path;

const char *contact_list;

const int HttpServiceTimeOut = 3;
int log_level;

std::map<int, int> agent_stat;
std::map<int, std::string> agent;
std::vector<compensiteitem> new_compensteitems;
std::vector<agentstatus> new_agent_stat;

int AgentPing(int agentid, std::string agentAddress) {
	log_info("Enter AgentPing");
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
	/*
	 * 对于connect来说，fd是阻塞的，则connect是阻塞的
	 * 设计了超时的fd对于connect来说也有影响，但是当超时时间
	 * 到时，若链接还没有完成，返回errno=115
	 */
	struct timeval tv = { 0, 500000 }; //500ms
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

int Connect() {
	mysql_init(mysql_inst);
	unsigned int timeout = 1;   //1 second
	mysql_options(mysql_inst, MYSQL_OPT_CONNECT_TIMEOUT,
			(const char *) &timeout);

	if (!mysql_real_connect(mysql_inst, host, username, password, db, port,
			path, 0)) {
		log_error("connect to db error: %s", mysql_error(mysql_inst));
		return -1;
	}
#ifdef DEBUG
	log_info("connected to db.\n");
#endif

	return 0;
}

int loadconfig() {
	char buf[1024];
	snprintf(buf, sizeof(buf) - 1, "select agent_id,ip,port from dtc_agent;");
	int rtn = Connect();
	if (rtn != 0) {
		return -1;
	}
	rtn = mysql_real_query(mysql_inst, buf, strlen(buf));
	if (rtn != 0) {
		log_error("select db error: %s", mysql_error(mysql_inst));

		if (rtn == CR_SERVER_GONE_ERROR || rtn == CR_SERVER_LOST
				|| rtn == CR_UNKNOWN_ERROR) {
			mysql_close(mysql_inst);
			rtn = Connect();
			if (rtn == 0) {
				rtn = mysql_real_query(mysql_inst, buf, strlen(buf));
			} else {
				//重新连接数据库失败，将time设置为0，返回
				log_error("reconnect db fail");
				return -1;
			}
		}
	}
	MYSQL_RES* result = mysql_use_result(mysql_inst);
	if (result == NULL) {
		return -2;
	}
	int agent_id;
	std::string ip;
	std::string port;
	std::string agentaddr;
	MYSQL_ROW row;
	int num_fields = mysql_num_fields(result);
	while ((row = mysql_fetch_row(result))) {
		for (int j = 0; j < num_fields; j++) {
			fprintf(stdout, "%s\n", row[j]);
		}
		agent_id = atoi(row[0]);
		ip = row[1];
		port = row[2];
		agentaddr = ip + ":" + port;
		agent[agent_id] = agentaddr;
	}
	mysql_free_result(result);
	return 0;
}

int writebackcompensiteitems() {
	log_debug("writebackcompensiteitems entered");
	struct stat sb;
	struct compensiteitem *ci_list;
	int ci_fd = open(COMPENSITELIST_FILE, O_RDWR | O_CREAT,0777);
	if (ci_fd == -1) {
		log_error("load compensite list from file failed");
		return -1;
	}
	if ((fstat(ci_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int ci_sz = sb.st_size / sizeof(struct compensiteitem);
	ftruncate(ci_fd,
			sb.st_size
					+ new_compensteitems.size()
							* sizeof(struct compensiteitem));
	ci_list = (struct compensiteitem *) mmap(0,
			sb.st_size
					+ new_compensteitems.size() * sizeof(struct compensiteitem),
			PROT_READ | PROT_WRITE,
			MAP_SHARED, ci_fd, 0);
	for (int i = ci_sz; i < (ci_sz + new_compensteitems.size()); i++) {
		ci_list[i] = new_compensteitems[i - ci_sz];
		log_debug("write back compensite item: agentid=%d,idmpt=%llu,status:%d",
				new_compensteitems[i - ci_sz].agentId,
				new_compensteitems[i - ci_sz].idmpt,
				new_compensteitems[i - ci_sz].state);
	}
	munmap(ci_list,
			sb.st_size
					+ new_compensteitems.size()
							* sizeof(struct compensiteitem));
	close(ci_fd);
	return 0;
}

/*
 * 尝试发送补偿列表的数据
 */
int sndcompensitelist() {
	log_debug("sndcompensitelist entered");
	int ret;
	struct stat sb;
	struct compensiteitem *ci_list;
	int ci_fd = open(COMPENSITELIST_FILE, O_RDWR);
	if (ci_fd == -1) {
		log_error("load compensite list from file failed");
		return -1;
	}
	if ((fstat(ci_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int ci_sz = sb.st_size / sizeof(struct compensiteitem);
	ci_list = (struct compensiteitem *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, ci_fd, 0);

	std::map<int, std::string>::iterator agent_iter;
	int i;
	for (i = 0; i < ci_sz; i++) {
		//ci_list[i].
		agent_iter = agent.find(ci_list[i].agentId);
		if (agent_iter == agent.end()) {
			continue;
		} else {
			if ((ret = ReportToAdmin(ci_list[i].agentId, ci_list[i].state,
					ci_list[i].idmpt)) == -1) {
				break;
			}
		}
	}
	int k = i;
	for (int j = 0; i < ci_sz; j++, i++) {
		ci_list[j] = ci_list[i];
	}
	munmap(ci_list, sb.st_size);
	ftruncate(ci_fd, sb.st_size - k * sizeof(struct compensiteitem));
	close(ci_fd);
	return 0;
}

/*
 * 打印补偿队列数据
 */
int pncompensitelist() {
	struct stat sb;
	struct compensiteitem *ci_list;
	int ci_fd = open(COMPENSITELIST_FILE, O_RDWR);
	if (ci_fd == -1) {
		log_error("load compensite list from file failed");
		return -1;
	}
	if ((fstat(ci_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int ci_sz = sb.st_size / sizeof(struct compensiteitem);
	ci_list = (struct compensiteitem *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, ci_fd, 0);

	int i;
	for (i = 0; i < ci_sz; i++) {
		printf("agentId:%d,state:%d,idmpt:%llu\n", ci_list[i].agentId,
				ci_list[i].state, ci_list[i].idmpt);
	}
	munmap(ci_list, sb.st_size);
	close(ci_fd);
	return 0;
}

/*
 * 从文件中load实例的状态inst_stat
 */

int loadagentstat() {
	struct stat sb;
	struct agentstatus *agent_stat_list;
	int agent_stat_fd = open(AGENTSTATUS_FILE, O_RDWR | O_CREAT,0777);
	if (agent_stat_fd == -1) {
		log_error("load agent stats from file failed");
		return -1;
	}
	if ((fstat(agent_stat_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int inst_sz = sb.st_size / sizeof(struct agentstatus);
	agent_stat_list = (struct agentstatus *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, agent_stat_fd, 0);
	for (int i = 0; i < inst_sz; i++) {
		agent_stat[agent_stat_list[i].agentid] = agent_stat_list[i].status;
	}
	munmap(agent_stat_list, sb.st_size);
	close(agent_stat_fd);
	return 0;
}

/*
 * 从文件中load实例的状态inst_stat
 */
int pnagentststat() {
	struct stat sb;
	struct agentstatus *agent_stat_list;
	int agent_stat_fd = open(AGENTSTATUS_FILE, O_RDWR);
	if (agent_stat_fd == -1) {
		log_error("load agent stats from file failed");
		return -1;
	}
	if ((fstat(agent_stat_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int inst_sz = sb.st_size / sizeof(struct agentstatus);
	agent_stat_list = (struct agentstatus *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, agent_stat_fd, 0);
	for (int i = 0; i < inst_sz; i++) {
		printf("agentid:%d,status:%d\n", agent_stat_list[i].agentid,
				agent_stat_list[i].status);
	}
	munmap(agent_stat_list, sb.st_size);
	close (agent_stat_fd);
	return 0;
}

/*
 * write new instance status back
 */
int writeagentstat() {
	struct agentstatus *agent_stat_list;
	int agent_stat_fd = open(AGENTSTATUS_FILE, O_RDWR);
	if (agent_stat_fd == -1) {
		log_error("load instance stats from file failed");
		return -1;
	}
	ftruncate(agent_stat_fd,
			new_agent_stat.size() * sizeof(struct agentstatus));
	agent_stat_list = (struct agentstatus *) mmap(0,
			new_agent_stat.size() * sizeof(struct agentstatus),
			PROT_READ | PROT_WRITE, MAP_SHARED, agent_stat_fd, 0);
	for (int i = 0; i < new_agent_stat.size(); i++) {
		agent_stat_list[i] = new_agent_stat[i];
		log_debug("add status file :agentid=%d,status=%d",
				new_agent_stat[i].agentid, new_agent_stat[i].status);
	}
	munmap(agent_stat_list, new_agent_stat.size() * sizeof(struct agentstatus));
	close(agent_stat_fd);
	return 0;
}

int ReportToAdmin(int agentid, int state, uint64_t idmpt) {
	char c[10];
	sprintf(c, "%d", agentid);
	std::string strAgentId = c;
	sprintf(c, "%d", state);
	std::string strstate = c;
	sprintf(c, "%llu", idmpt);
	std::string stridmpt = c;
	BuffV buf;
	std::string strResp;
	int ret;
	std::string reportUrl = adminurl;   // "http://127.0.0.1/stm/dtcError";
	std::string strbody = "agentId=" + strAgentId + "&state=" + strstate
			+ "&idmpt=" + stridmpt;
	log_debug("%s", strbody.c_str());
	CurlHttp curlHttp;
	curlHttp.SetTimeout(HttpServiceTimeOut);
	curlHttp.SetHttpParams(strbody.data());
	//"Content-Type: application/x-www-form-urlencoded"
	ret = curlHttp.HttpRequest(reportUrl, &buf, false,
			"Content-Type: application/x-www-form-urlencoded");
	strResp = buf.Ptr();
	log_info("strResp:%s", strResp.c_str());
	if (ret != 0) {
		log_error("report http error!~~~~~~");
		return -1;
	}
	strResp = buf.Ptr();
	if (strResp == "0") {
		return 0;
	} else {
		log_debug("return value error");
		return -1;
	}
}

bool ReportToAlarm(std::string ip) {
	std::string report_url=alarmurl;//"http://monitor.m.jd.com/tools/alarm/api"
	Json::Value innerBody;
	innerBody["alarm_list"] = contact_list;
	std::stringstream oss;
	innerBody["alarm_title"] = "agent ping fail";
	std::string report_content="ip: "+ip+" agent has reply ping message 3 times";
	log_debug("%s",report_content.c_str());
	innerBody["alarm_content"] = report_content.c_str();

	Json::Value body;
	body["app_name"] = "agent_monitor";
	body["alarm_type"] = "sms";
	body["data"] = innerBody;
	Json::FastWriter writer;
	std::string strBody = writer.write(body);
	log_info("HttpBody = [%s]", strBody.c_str());

	const unsigned int HttpServiceTimeOut = 3;
	CurlHttp curlHttp;
	std::stringstream reqStream;
	reqStream << "req=" << strBody;
	log_info("m_url: %s  ,reqStream = [%s]", report_url.c_str(),reqStream.str().c_str());
	curlHttp.SetHttpParams("%s", reqStream.str().c_str());
	curlHttp.SetTimeout(HttpServiceTimeOut);

	BuffV buf;

	int ret = curlHttp.HttpRequest(report_url, &buf, false,"application/x-www-form-urlencoded");
	if (ret != 0) {
		log_error("curlHttp.HttpRequest Error! ret=%d", ret);
		return false;
	}

	std::string strRsponse = buf.Ptr();
	log_info("strRsponse:%s", strRsponse.c_str());

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(strRsponse.c_str(), root)) {
		log_error("parse Json failed, strRsponse:%s", strRsponse.c_str());
		return false;
	}

	std::string retStatus = root["status"].asString();
	log_info("retStatus:%s", retStatus.c_str());

	if (retStatus == "success") {
		log_info("report to alarm_platform success.");
		return true;
	} else {
		std::string strMessage = root["message"].asString();
		log_error("report to alarm_platform failed, strRetCode:%s.",
				strMessage.c_str());
		return false;
		//exit(-1);
	}
}

void show_usage(const char *prog) {
	printf("%s [-c][-s]\n", prog);
}

int main(int argc, char** argv) {
	int c = -1;
	while ((c = getopt(argc, argv, "cs")) != -1) {
		switch (c) {
		case '?':
			show_usage(argv[0]);
			exit(1);
		case 'c':
			pncompensitelist();
			exit(0);
		case 's':
			pnagentststat();
			exit(0);
		}
	}
	if (UnixSocketLock("agentmonitor") < 0) {
		std::cout << " already started " << std::endl;
		return -1;
	}
	if (gConfigObj->ParseConfig(agentFile, "monitor", true)) {
		return -1;
	}
	//get db config
	host = gConfigObj->GetStrVal("monitor", "Host");
	port = gConfigObj->GetIntVal("monitor", "Port");
	username = gConfigObj->GetStrVal("monitor", "UserName");
	password = gConfigObj->GetStrVal("monitor", "PassWord");
	db = gConfigObj->GetStrVal("monitor", "DB");
	path = gConfigObj->GetStrVal("monitor", "Path");
	//get report url
	alarmurl = gConfigObj->GetStrVal("monitor", "AgentPingMonitorReportURL");
	adminurl = gConfigObj->GetStrVal("monitor", "AdminReportURLAgent");
	//get contact list
	contact_list=gConfigObj->GetStrVal("monitor", "ContactList");
	//get log_level
	log_level = gConfigObj->GetIntVal("monitor", "LogLevel");
	_init_log_("dtc_monitor", "../log");
	_set_log_level_(log_level);
	int ret = 0;
	ret = loadconfig();
	if (ret < 0) {
		if (ret == -1) {
			log_error("get agent from db error");
			return -1;
		} else if (ret == -2) {
			log_debug("there is no agent in db ");
			return 0;
		}
	}
	ret = loadagentstat();
	if (ret < 0) {
		log_error("load agent state from file error");
		return -1;
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	std::map<int, int>::iterator state_iter;
	std::map<int, std::string>::iterator agent_iter = agent.begin();
	for (; agent_iter != agent.end(); agent_iter++) {
		state_iter = agent_stat.find(agent_iter->first);
		ret = AgentPing(agent_iter->first, agent_iter->second);
		if (ret < 0) {
			ReportToAlarm(agent_iter->second);
			if (state_iter == agent_stat.end() || state_iter->second == 1) {
				agentstatus as;
				as.agentid = agent_iter->first;
				as.status = 2;
				new_agent_stat.push_back(as);
				compensiteitem compitem;
				compitem.agentId = agent_iter->first;
				compitem.state = 2;
				compitem.idmpt = tv.tv_sec;
				new_compensteitems.push_back(compitem);
			} else {
				agentstatus as;
				as.agentid = agent_iter->first;
				as.status = 2;
				new_agent_stat.push_back(as);
			}
		} else {
			if (state_iter->second == 2) {
				agentstatus as;
				as.agentid = agent_iter->first;
				as.status = 1;
				new_agent_stat.push_back(as);
				compensiteitem compitem;
				compitem.agentId = agent_iter->first;
				compitem.state = 1;
				compitem.idmpt = tv.tv_sec;
				new_compensteitems.push_back(compitem);
			} else {
				agentstatus as;
				as.agentid = agent_iter->first;
				as.status = 1;
				new_agent_stat.push_back(as);
			}
		}
	}
	writeagentstat();
	writebackcompensiteitems();
	sndcompensitelist();
	return 0;

}

