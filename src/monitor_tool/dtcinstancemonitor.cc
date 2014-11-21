#include "dtcinstancemonitor.h"

#define MONITOR_CONF_NAME  "../conf/monitor.conf"
#define COMPENSITELIST_FILE "../stat/dtccompensitelist.dat"
#define DTCINSTANCESTATUS_FILE "../stat/dtcinstancestatus.dat"
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

std::map<int, std::map<std::string, int> > inst_stat;
std::map<int, std::map<std::string, std::string> > inst;
std::vector<compensiteitem> new_compensteitems;
std::vector<instancestatus> new_inst_stat;

int DtcPing(std::string dtcAddress) {
	log_info("%s", dtcAddress.c_str());
	TTC::Server sever;
	sever.StringKey();
	sever.SetTableName("*");
	sever.SetAddress(dtcAddress.c_str());
	sever.SetTimeout(3);
	int ret = -1;
	int times = 0;
	while ((ret != 0) && (ret != -TTC::EC_TABLE_MISMATCH)) {
		if (times == 3) {
			break;
		}
		ret = sever.Ping();
		times++;
	}
	if ((ret != 0 && ret != -TTC::EC_TABLE_MISMATCH))
		return -1;
	else
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

	std::map<int, std::map<std::string, std::string> >::iterator module_item;
	std::map<std::string, std::string>::iterator inst_item;
	int i;
	for (i = 0; i < ci_sz; i++) {
		//ci_list[i].
		module_item = inst.find(ci_list[i].busId);
		if (module_item == inst.end()) {
			continue;
		} else {
			std::string strInsId = ci_list[i].insId;
			inst_item = (module_item->second).find(strInsId);
			if (inst_item == (module_item->second).end()) {
				continue;
			} else {
				if ((ret = ReportToAdmin(ci_list[i].busId, ci_list[i].addr,
						ci_list[i].state, ci_list[i].idmpt)) == -1) {
					break;
				}
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
		printf("busId:%d,instId:%d,ip:%s,state:%d,idmpt:%llu\n",
				ci_list[i].busId, ci_list[i].insId, ci_list[i].addr,
				ci_list[i].state, ci_list[i].idmpt);
	}
	munmap(ci_list, sb.st_size);
	close(ci_fd);
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
		log_debug(
				"write back compensite item: busid=%d,instid=%s,idmpt=%llu,ip=%s,status:%d",
				new_compensteitems[i - ci_sz].busId,
				new_compensteitems[i - ci_sz].insId,
				new_compensteitems[i - ci_sz].idmpt,
				new_compensteitems[i - ci_sz].addr,
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
 * 从文件中load实例的状态inst_stat
 */

int loadinststat() {
	struct stat sb;
	struct instancestatus *inst_stat_list;
	int inst_stat_fd = open(DTCINSTANCESTATUS_FILE, O_RDWR | O_CREAT,0777);
	if (inst_stat_fd == -1) {
		log_error("load instance stats from file failed");
		return -1;
	}
	if ((fstat(inst_stat_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int inst_sz = sb.st_size / sizeof(struct instancestatus);
	inst_stat_list = (struct instancestatus *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, inst_stat_fd, 0);
	for (int i = 0; i < inst_sz; i++) {
		std::string strInsId = inst_stat_list[i].insId;
		inst_stat[inst_stat_list[i].busId][strInsId] = inst_stat_list[i].status;
	}
	munmap(inst_stat_list, sb.st_size);
	close(inst_stat_fd);
	return 0;
}

/*
 * 从文件中load实例的状态inst_stat
 */

int pninststat() {
	struct stat sb;
	struct instancestatus *inst_stat_list;
	int inst_stat_fd = open(DTCINSTANCESTATUS_FILE, O_RDWR);
	if (inst_stat_fd == -1) {
		log_error("load instance stats from file failed");
		return -1;
	}
	if ((fstat(inst_stat_fd, &sb)) == -1) {
		log_error("get file stat failed");
		return -1;
	}
	int inst_sz = sb.st_size / sizeof(struct instancestatus);
	inst_stat_list = (struct instancestatus *) mmap(0, sb.st_size,
	PROT_READ | PROT_WRITE,
	MAP_SHARED, inst_stat_fd, 0);
	for (int i = 0; i < inst_sz; i++) {
		printf("busId:%d,insId:%s,status:%d\n", inst_stat_list[i].busId,
				inst_stat_list[i].insId, inst_stat_list[i].status);
	}
	munmap(inst_stat_list, sb.st_size);
	close(inst_stat_fd);
	return 0;
}

/*
 * write new instance status back
 */
int writeinststat() {
	struct instancestatus *inst_stat_list;
	int inst_stat_fd = open(DTCINSTANCESTATUS_FILE, O_RDWR);
	if (inst_stat_fd == -1) {
		log_error("load instance stats from file failed");
		return -1;
	}
	ftruncate(inst_stat_fd,
			new_inst_stat.size() * sizeof(struct instancestatus));
	inst_stat_list = (struct instancestatus *) mmap(0,
			new_inst_stat.size() * sizeof(struct instancestatus),
			PROT_READ | PROT_WRITE, MAP_SHARED, inst_stat_fd, 0);
	for (int i = 0; i < new_inst_stat.size(); i++) {
		inst_stat_list[i] = new_inst_stat[i];
		log_debug("add status file :businessid=%d,instanceid=%s,status=%d",
				new_inst_stat[i].busId, new_inst_stat[i].insId,
				new_inst_stat[i].status);
	}
	munmap(inst_stat_list,
			new_inst_stat.size() * sizeof(struct instancestatus));
	close(inst_stat_fd);
	return 0;
}

/*
 * connect to db to get config
 */
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

/*
 *
 * 解析配置文件inst,log_level
 */
int loadconfig() {
	char buf[1024];
	snprintf(buf, sizeof(buf) - 1,
			"select business_id,instance_id,ip,port,state from dtc_instance;");
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
	int m_id;
	std::string Inst_Id;
	std::string ip;
	std::string port;
	std::string dtcaddr;
	std::string state;
	MYSQL_ROW row;
	int num_fields = mysql_num_fields(result);
	while ((row = mysql_fetch_row(result))) {
		for (int j = 0; j < num_fields; j++) {
			fprintf(stdout, "%s\n", row[j]);
		}
		m_id = atoi(row[0]);
		Inst_Id = row[1];
		ip = row[2];
		port = row[3];
		state= row[4];
		dtcaddr = ip + ":" + port + "/tcp";
		if(state=="2" || state=="4")
			inst[m_id][Inst_Id] = dtcaddr;
	}
	mysql_free_result(result);
	return 0;
}

int ReportToAdmin(int busId, char addr[], int state, uint64_t idmpt) {
	std::string ip = addr;
	char c[10];
	sprintf(c, "%d", busId);
	std::string strbusId = c;
	sprintf(c, "%d", state);
	std::string strstate = c;
	sprintf(c, "%llu", idmpt);
	std::string stridmpt = c;
	BuffV buf;
	std::string strResp;
	int ret;
	std::string reportUrl = adminurl;   // "http://127.0.0.1/stm/dtcError";
	std::string strbody = "busId=" + strbusId + "&ip=" + ip + "&state="
			+ strstate + "&idmpt=" + stridmpt;
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
	innerBody["alarm_title"] = "dtc ping fail";
	std::string report_content="ip: "+ip+" dtc instance has reply ping message 3 times";
	log_debug("%s",report_content.c_str());
	innerBody["alarm_content"] = report_content.c_str();

	Json::Value body;
	body["app_name"] = "dtc_monitor";
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
			pninststat();
			exit(0);
		}
	}
	if (UnixSocketLock("dtcmonitor") < 0) {
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
	//get contact list
	contact_list=gConfigObj->GetStrVal("monitor", "ContactList");
	//get report url
	alarmurl = gConfigObj->GetStrVal("monitor", "DtcPingMonitorReportURL");
	adminurl = gConfigObj->GetStrVal("monitor", "AdminReportURLDtc");
	//get log_level
	log_level = gConfigObj->GetIntVal("monitor", "LogLevel");
	_init_log_("dtc_monitor", "../log");
	_set_log_level_(log_level);
	int ret = 0;
	ret = loadconfig();
	if (ret < 0) {
		if (ret == -1) {
			log_error("get instance from db error");
			return -1;
		} else if (ret == -2) {
			log_debug("there is no instance in db ");
			return 0;
		}
	}
	ret = loadinststat();
	if (ret < 0) {
		log_error("load instance state from file error");
		return -1;
	}

	int new_module_sig;
	int new_inst_sig;
	struct timeval tv;
	std::map<int, std::map<std::string, std::string> >::iterator module_itor =
			inst.begin();
	for (; module_itor != inst.end(); module_itor++) {
		new_module_sig = 0;
		std::map<int, std::map<std::string, int> >::iterator module_stat_itor =
				inst_stat.find(module_itor->first);
		if (module_stat_itor == inst_stat.end()) {
			new_module_sig = 1;
			log_debug("It's a new module been added");
		}
		std::map<std::string, std::string>::iterator inst_itor =
				(module_itor->second).begin();
		for (; inst_itor != (module_itor->second).end(); inst_itor++) {
			new_inst_sig = 0;
			int status = 0;
			if (new_module_sig == 0) {
				std::map<std::string, int>::iterator inst_stat_itor =
						(module_stat_itor->second).find(inst_itor->first);
				if (inst_stat_itor == (module_stat_itor->second).end()) {
					log_debug("It's a new instance add now");
					new_inst_sig = 1;
				} else {
					status = inst_stat_itor->second;
				}
			} else {
				new_inst_sig = 1;
			}
			ret = DtcPing(inst_itor->second);
			if (ret == -1) {
				/*
				 * 失败的情况下如果是新加入的instance或则instanced的状态为2 则不需要上报
				 */
				//通知告警
				ReportToAlarm(inst_itor->second);
				if (new_inst_sig == 1 || status == 0) {
					instancestatus it;
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 0;
					new_inst_stat.push_back(it);
				} else if (status == 2) {
					instancestatus it;
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 2;
					new_inst_stat.push_back(it);
				} else {
					gettimeofday(&tv, NULL);
					compensiteitem ci;
					instancestatus it;
					ci.busId = module_itor->first;
					const char *d = ":";
					char *p;
					char *sourcestr =
							const_cast<char*>((inst_itor->second).c_str());
					p = strtok(sourcestr, d);
					strcpy(ci.addr, p);
					ci.state = 4;
					ci.idmpt = tv.tv_sec;
					strcpy(ci.insId, (inst_itor->first).c_str());
					new_compensteitems.push_back(ci);
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 2;
					new_inst_stat.push_back(it);
				}
			} else {
				/*
				 *成功的状态下，如果是新的instance需要扭转状态
				 */
				if (new_inst_sig == 1 || status == 0) {
					log_debug("ping success,but it's a new instance");
					instancestatus it;
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 1;
					new_inst_stat.push_back(it);
				} else if (status == 1) {    //对于原状态是成功的不需要上报数据
					log_debug("ping success,instance in success status ready");
					instancestatus it;
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 1;
					new_inst_stat.push_back(it);
				} else {
					gettimeofday(&tv, NULL);
					compensiteitem ci;
					instancestatus it;
					ci.busId = module_itor->first;
					const char *d = ":";
					char *p;
					char *sourcestr =
							const_cast<char*>((inst_itor->second).c_str());
					p = strtok(sourcestr, d);
					strcpy(ci.addr, p);
					ci.state = 2;
					ci.idmpt = tv.tv_sec;
					strcpy(ci.insId, (inst_itor->first).c_str());
					new_compensteitems.push_back(ci);
					it.busId = module_itor->first;
					strcpy(it.insId, (inst_itor->first).c_str());
					it.status = 1;
					new_inst_stat.push_back(it);
				}
			}

		}
	}
	writeinststat();
	writebackcompensiteitems();
	sndcompensitelist();
	return 0;
}
