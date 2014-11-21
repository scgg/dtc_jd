/*
 * =====================================================================================
 *
 *       Filename:  main.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/11/2010 08:50:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <stdint.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tinyxml.h"

#include "poll_thread.h"
#include "log.h"
#include "admin_acceptor.h"
#include "config_file.h"
#include "heartbeat.h"
#include "admin.h"
#include "sock_util.h"

#include "module.h"
#include "business_module.h"
#include "constant.h"
#include "stat_agent.h"
#include "admin_protocol.pb.h"
#include "poll_thread_group.h"
#include "stat_definition.h"
#include "agent_alert.h"
#include "thread_cpu_stat.h"
#include "Version.h"
#include "CFdDispacher.h"
#include "plugin_agent_mgr.h"
#include "plugin_global.h"

#ifndef SVN_REVISION
#error SVN_REVISION not defined!
#endif

#define STR(x)  #x
#define STR2(x) STR(x)
#define REVISION    "build " STR2(SVN_REVISION)

const char *versionStr = "2.0.0 build " STR2(SVN_REVISION);

char gConfig[256] = "../conf/agent.xml";
#define AGENT_CONF_NAME  "../conf/agent.conf"
char agentFile[256] = AGENT_CONF_NAME;

char gAlertFile[256] = "../conf/contacts.xml";
int gClientIdleTime;

static int enablePlugin = 0;
static int initPlugin = 0;
static CPluginAgentManager *plugin_mgr;
int gMaxConnCnt = 10;

CBussinessModuleMgr *gBusinessModuleMgr;
CFdDispacher *gFdDispacher;

CPollThreadGroup *gPollThreadGroup;
CConfig* gConfigObj = new CConfig;
CPollThread * gAgentThread;
std::string gAdminAddr;
std::string gMasterAddr;

static int plugin_start(void) {
	initPlugin = 0;

	plugin_mgr = CPluginAgentManager::Instance();
	if (NULL == plugin_mgr) {
		log_error("create CPluginManager instance failed.");
		return -1;
	}

	// plugin::open
	if (plugin_mgr->open() != 0) {
		log_error("init plugin manager failed.");
		return -1;
	}

	initPlugin = 1;

	return 0;
}

static int plugin_stop(void) {
	plugin_mgr->close();
	CPluginAgentManager::Destroy();
	plugin_mgr = NULL;

	return 0;
}

int BuildModuleMgr(TiXmlElement *modules) {
	gBusinessModuleMgr = new CBussinessModuleMgr();
	if (NULL == gBusinessModuleMgr) {
		log_error("no mem build module manager");
		return -1;
	}

	TiXmlElement *m = modules->FirstChildElement("MODULE");
	while (m) {
		uint32_t id = atoi(m->Attribute("Id"));
		std::string listenOn = m->Attribute("ListenOn");
		CStatManager * manager = CreateStatManager(id);
		if (NULL == manager) {
			log_error("create statManager error");
			return -1;
		}
		CStatManagerContainerThread::getInstance()->AddStatManager(id, manager);

		CModule *module = new CModule(id, gAgentThread, gPollThreadGroup,
				gConfigObj, manager, gFdDispacher);
		if (gBusinessModuleMgr->AddModule(module) < 0) {
			log_error("add module %d failed", id);
			return -1;
		}

		if (module->Listen(m->Attribute("ListenOn")) < 0) {
			log_error("error when listen on %s", m->Attribute("ListenOn"));
			return -1;
		}

		const char *accessToken = m->Attribute("AccessToken");
		log_info("accessToken:%s", accessToken);
		if (accessToken)
			module->SetAccessToken(accessToken);

		const char *strCascadeKey = m->Attribute("Cascade");
		uint32_t cascadekey = 0;
		if (strCascadeKey) {
			cascadekey = atoi(strCascadeKey);
		}
		const char *cascadeAgent = m->Attribute("CascadeAG");
		const char *strCascadePercent = m->Attribute("Percent");
		uint32_t cascadePercent = 0;
		if (strCascadePercent) {
			cascadePercent = atoi(strCascadePercent);
		}
		log_debug("cascadekey:%d,cascadeAgent:%s,cascadePercent:%d", cascadekey,
				cascadeAgent, cascadePercent);
		if (cascadekey != 0) {
			if (cascadeAgent)
				module->SetCascadeAgentServer("cascadeagent", cascadeAgent,
						cascadePercent);
			else
				log_error("cascadeAgent is null when use cascade mode!");
		}
		const char *strHotBackupAddr;
		const char *strMode;
		int mode;
		TiXmlElement *cacheNode = m->FirstChildElement("CACHEINSTANCE");
		while (cacheNode) {
			log_debug("Add cache node %s %s", cacheNode->Attribute("Name"),
					cacheNode->Attribute("Addr"));
			strHotBackupAddr = cacheNode->Attribute("HotBackupAddr");
			if (!strHotBackupAddr) {
				strHotBackupAddr = "";
			}
			strMode = cacheNode->Attribute("Locate");
			if (!strMode) {
				strMode = "master";
			}
			if (strcmp(strMode, "master") == 0) {
				mode = 0;
			}
			else if(strcmp(strMode, "slave") == 0) {
				mode = 1;
			}
			else
			{
				log_error("error type of locate");
				return -1;
			}

			for (int i = 0; i < gPollThreadGroup->GetPollThreadSize(); i++) {
				module->AddCacheServer(gPollThreadGroup->GetPollThread(i),
						cacheNode->Attribute("Name"),
						cacheNode->Attribute("Addr"), -1, strHotBackupAddr,
						mode);
			}
			cacheNode = cacheNode->NextSiblingElement("CACHEINSTANCE");
		}
		m = m->NextSiblingElement("MODULE");
	}
	return 0;
}

int AGParseConfig() {
	TiXmlDocument configdoc;
	if (!configdoc.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gConfig,
				configdoc.ErrorDesc(), configdoc.ErrorRow(),
				configdoc.ErrorCol());
		return -1;
	}

	TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *agentconf = rootnode->FirstChildElement("AGENT_CONFIG");
	if (NULL == agentconf) {
		log_error("agent conf miss");
		return -1;
	}

	gClientIdleTime = atoi(agentconf->Attribute("ClientIdleTime"));
	const char *logLevel = agentconf->Attribute("LogLevel");

	if (logLevel)
		_set_log_level_(atoi(logLevel));

	const char *adminListenAddr = agentconf->Attribute("AdminListen");
	gMasterAddr = agentconf->Attribute("MasterAddr");
	uint32_t agentId = atoi(agentconf->Attribute("AgentId"));
	gCurrentConfigureVersion = atoi(agentconf->Attribute("Version"));
	gCurrentConfigureCksum = ConfigGetCksum();
	int groupSize = gPollThreadGroup->GetPollThreadSize();
	gFdDispacher = new CFdDispacher[groupSize];
	for (int i = 0; i < groupSize; i++) {
		gFdDispacher[i].SetOwnerThread(gPollThreadGroup->GetPollThread(i));
		gFdDispacher[i].AttachPoller(gPollThreadGroup->GetPollThread(i));
	}

	TiXmlElement *modules = rootnode->FirstChildElement("BUSINESS_MODULE");
	if (!modules) {
		log_error("empty module");
		return -1;
	}

	if (BuildModuleMgr(modules) < 0) {
		log_error("build module mgr failed");
		return -1;
	}

	// Plugin
	TiXmlElement *plugconf = rootnode->FirstChildElement("AGENT_PLUGIN");
	if (NULL != plugconf) {
		enablePlugin = 0;
		if (NULL != plugconf->Attribute("EnablePlugin")) {
			enablePlugin = atoi(plugconf->Attribute("EnablePlugin"));
		}
		const char* plugin_log_path = plugconf->Attribute("LogPath");
		if (NULL == plugin_log_path) {
			plugin_log_path = (const char*) "../log";
		}
		CPluginAgentManager::Instance()->plugin_log_path = strdup(
				plugin_log_path);
		const char* plugin_log_name = plugconf->Attribute("LogName");
		if (NULL == plugin_log_name) {
			plugin_log_name = (const char*) "plugin_";
		}
		CPluginAgentManager::Instance()->plugin_log_name = strdup(
				plugin_log_name);
		int plugin_log_level = 7;
		if (NULL != agentconf->Attribute("LogLevel")) {
			plugin_log_level = atoi(agentconf->Attribute("LogLevel"));
		}
		CPluginAgentManager::Instance()->plugin_log_level = plugin_log_level;
		int plugin_log_size = 1 << 28;
		if (NULL != plugconf->Attribute("LogSize")) {
			plugin_log_size = atoi(plugconf->Attribute("LogSize"));
		}
		CPluginAgentManager::Instance()->plugin_log_size = plugin_log_size;

		const char* plugin_name = plugconf->Attribute("PluginName");
		CPluginAgentManager::Instance()->set_plugin_name(plugin_name);

		const char* plugin_conf_file = plugconf->Attribute("PluginConfigFile");
		CPluginAgentManager::Instance()->set_plugin_conf_file(plugin_conf_file);

		int plugin_worker_number = 1;
		if (NULL != plugconf->Attribute("PluginWorkerNumber")) {
			plugin_worker_number = atoi(
					plugconf->Attribute("PluginWorkerNumber"));
		}
		CPluginAgentManager::Instance()->plugin_worker_number =
				plugin_worker_number;

		int plugin_timer_notify_interval = 10;
		if (NULL != plugconf->Attribute("PluginTimerNotifyInterval")) {
			plugin_timer_notify_interval = atoi(
					plugconf->Attribute("PluginTimerNotifyInterval"));
		}
		CPluginAgentManager::Instance()->plugin_timer_notify_interval =
				plugin_timer_notify_interval;

		if (NULL != plugconf->Attribute("IdleTimeout")) {
			CPluginGlobal::_idle_timeout = atoi(
					plugconf->Attribute("IdleTimeout"));
		}
		if (NULL != plugconf->Attribute("SingleIncomingThread")) {
			CPluginGlobal::_single_incoming_thread = atoi(
					plugconf->Attribute("SingleIncomingThread"));
		}
		if (NULL != plugconf->Attribute("MaxListenCount")) {
			CPluginGlobal::_max_listen_count = atoi(
					plugconf->Attribute("MaxListenCount"));
		}
		if (NULL != plugconf->Attribute("MaxRequestWindow")) {
			CPluginGlobal::_max_request_window = atoi(
					plugconf->Attribute("MaxRequestWindow"));
		}
		if (NULL != plugconf->Attribute("PluginAddr")) {
			CPluginGlobal::_bind_address = strdup(
					plugconf->Attribute("PluginAddr"));
		}
		if (NULL != plugconf->Attribute("UdpRecvBufferSize")) {
			CPluginGlobal::_udp_recv_buffer_size = atoi(
					plugconf->Attribute("UdpRecvBufferSize"));
		}
		if (NULL != plugconf->Attribute("UdpSendBufferSize")) {
			CPluginGlobal::_udp_send_buffer_size = atoi(
					plugconf->Attribute("UdpSendBufferSize"));
		}
	}

//	CAgentHeartBeatWorker *heartbeatWorker = new CAgentHeartBeatWorker(agentId,
//			gMasterAddr.c_str(), gAgentThread);
//	heartbeatWorker->Build();

	CAdminAcceptor *acceptor = new CAdminAcceptor();
	if (acceptor->Build(adminListenAddr, gAgentThread, gPollThreadGroup,
			gConfigObj, gFdDispacher) < 0) {
		log_error("build admin acceptor failed");
		return -1;
	}

	return 0;
}

int AGparseContacts() {
	TiXmlDocument configdoc;
	if (!configdoc.LoadFile(gAlertFile)) {
		log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gAlertFile,
				configdoc.ErrorDesc(), configdoc.ErrorRow(),
				configdoc.ErrorCol());
		return -1;
	}

	TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *contact = rootnode->FirstChildElement("Contacts");
	if (NULL == contact) {
		log_error("phone_list xml miss");
		return -1;
	}

	TiXmlElement *urls = rootnode->FirstChildElement("Urls");
	if (NULL == urls) {
		log_error("http request url xml miss");
		return -1;
	}

	TiXmlElement *netDev = rootnode->FirstChildElement("NetDev");
	if (NULL == netDev) {
		log_error("netDev xml miss");
		return -1;
	}
	const char* mobile_phone = contact->Attribute("mobile_phone");
	AgentAlert::GetInstance().setContacts(std::string(mobile_phone));

	const char* url = urls->Attribute("alarm_url");
	AgentAlert::GetInstance().setUrl(std::string(url));

	const char* net_flow = netDev->Attribute("net_flow");
	AgentAlert::GetInstance().setNetFlow(atoi(net_flow));

	const char* ethernet = netDev->Attribute("ethernet");
	AgentAlert::GetInstance().setEthernet(std::string(ethernet));

	const char* ipaddr = netDev->Attribute("ipaddr");
	AgentAlert::GetInstance().setIpaddr(std::string(ipaddr));
	return 0;
}

int AgentThreadSetup() {
	gAgentThread = new CPollThread("agent");
	CThread::SetAutoConfigInstance(gConfigObj->GetAutoConfigInstance("agent"));
	if (NULL == gAgentThread) {
		log_error("create gAdminthread error, no mem");
		return -1;
	}
	gAgentThread->SetMaxPollers(100000);
	gAgentThread->InitializeThread();

	return 0;
}
/*
 *
 */
int PollThreadGroupSetup() {
	gPollThreadGroup = new CPollThreadGroup("agentworker");
	gPollThreadGroup->Start(gConfigObj->GetIntVal("agent", "ThreadNum", 8),
			100000);
	AgentAlert::GetInstance().setWorkerNum(
			gConfigObj->GetIntVal("agent", "ThreadNum", 8));
	//gPollThreadGroup->Start(8,100000);
	return 0;
}

static volatile int agentRunning = 1;

static void sigterm_handler(int signo) {
	agentRunning = 0;
}

static
void AgentDaemonStart() {
	struct sigaction sa;
	sigset_t sset;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
}

int DaemonSetFdLimit(int maxfd) {
	struct rlimit rlim;
	if (maxfd) {
		/* raise open files */
		rlim.rlim_cur = maxfd;
		rlim.rlim_max = maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
			log_error("Increase FdLimit failed, set val[%d] errmsg[%s]", maxfd,
					strerror(errno));
			return -1;
		}
	}
	return 0;
}

void show_usage(const char *prog) {
	printf("%s [-v]", prog);
}

int main(int argc, char ** argv) {
	int c = -1;
	while ((c = getopt(argc, argv, "v")) != -1) {
		switch (c) {
		case '?':
			show_usage(argv[0]);
			exit(1);
		case 'v':
			printf("dtcagent version: %s\n", GetCurrentVersion().c_str());
			printf("dtcagent last commit date: %s\n",
					GetLastModifyDate().c_str());
			printf("dtcagent last commit author: %s\n",
					GetLastAuthor().c_str());
			printf("dtcagent compile date: %s\n", GetCompileDate().c_str());
			exit(0);
		}
	}
	if (gConfigObj->ParseConfig(agentFile, "agent", true)) {
		return -1;
	}
	if (!IsConfigValid()) {
		printf("invalid config file, please check.\n");
		exit(1);
	}
#ifdef DEBUG
	printf("DEBUG on\n");
#endif
	signal(SIGPIPE, SIG_IGN);
	//no change dir
	daemon(1, 0);

	_init_log_("dtc_agent", "../log");

	//璁剧疆鏈�ぇ鐨凢D鏁扮洰
	if (DaemonSetFdLimit(100000)) {
		log_error("set max open fd error");
		return -1;
	}
	///for agent alert
	AGparseContacts();
	while (true) {
		pid_t p = fork();
		if (p == 0) {
			//_init_log_("ttc_agent", "../log");
			mkdir("../stat", 0777);
			_set_log_level_(7);
			CStatManagerContainerThread::getInstance()->StartBackgroundThread();
			AgentDaemonStart();
			AgentThreadSetup();
			PollThreadGroupSetup();
			if (AGParseConfig() < 0)
				return -1;

			log_info("config parsed...");
			log_info("TTCAgent start...");
			gAgentThread->RunningThread();
			gPollThreadGroup->RunningThreads();

			//init plugin
			if (enablePlugin) {
				if (plugin_start() < 0) {
					return -1;
				}
			}

			////涓嬮潰浠ｇ爜涓荤嚎绋嬬敤鏉ョ洃鎺gent杩愯鎯呭喌///////
			AgentAlert::GetInstance().alertInit();
			while (agentRunning) {
				//pause();
				sleep(10);
				if (!agentRunning)
					break;

				/* 鎵弿姣忎釜绾跨▼鐨刢pu浣跨敤锛屽鏋滆秴杩囬厤缃槇鍊硷紝鍚戣繍缁村钩鍙扮煭淇″憡璀�*/
				unsigned int ratio = 0;
				std::string excepThreadName;
				bool ret = AgentAlert::GetInstance().ScanCpuThreadStat(ratio,
						excepThreadName);
				if (ret) {
					log_alert("cpu thread overload ratio: %d  threadName: %s",
							ratio / 100, excepThreadName.c_str());
					AgentAlert::GetInstance().AlarmToPlatform(ratio, 0,
							excepThreadName, CPU_ALARM);
				}

				/* 鎵弿杩涚▼鎵撳紑鐨刦d鍙ユ焺鏁帮紝濡傛灉瓒呰繃閰嶇疆闃堝�锛屽悜杩愮淮骞冲彴鐭俊鍛婅 */
				unsigned int statfd =
						AgentAlert::GetInstance().ScanProcessOpenningFD();
				//log_alert("Fd_count: %u, fd_thresold: %u", statfd,(AgentAlert::GetInstance().getFdLimit()*8)/10);
				if (statfd
						> ((AgentAlert::GetInstance().getFdLimit() * 8) / 10)) {
					log_alert("openning fd overload[current=%d threshold=%d]",
							statfd,
							(AgentAlert::GetInstance().getFdLimit() * 8) / 10);
					AgentAlert::GetInstance().AlarmToPlatform(statfd,
							(AgentAlert::GetInstance().getFdLimit() * 8) / 10,
							"", FD_ALARM);
				} //if

				/*鎵弿鍐呭瓨宸茬粡浣跨敤澶у皬锛屽鏋滆秴杩囬厤缃槇鍊硷紝鍚戣繍缁村钩鍙扮煭淇″憡璀�*/
				unsigned int total = 0, used = 0;
				bool excessLimit = AgentAlert::GetInstance().ScanMemoryUsed(
						total, used);
				if (excessLimit) {
					log_alert(
							"used memory overload total memory [used=%d total=%d]",
							used, total);
					AgentAlert::GetInstance().AlarmToPlatform(used,
							(total * 8) / 10, "", MEM_ALARM);
				} //if

				/*鎵弿缃戠粶娴侀噺锛屽鏋滆秴杩囬厤缃槇鍊硷紝 鍚戣繍缁村钩鍙扮煭淇″憡璀�*/
				unsigned long flowPerSecond = 0, flowThreshold = 0;
				AgentAlert::GetInstance().ScanNetDevFlowing(flowPerSecond,
						flowThreshold);
				if (flowPerSecond > flowThreshold) {
					log_alert(
							"current network flow overload  maximum network flow threshold [current=%lu threshold=%lu]",
							flowPerSecond, flowThreshold);
					AgentAlert::GetInstance().AlarmToPlatform(flowPerSecond,
							flowThreshold, "", NET_ALARM);
				}

			} //while

			//stop plugin
			if (enablePlugin && initPlugin) {
				plugin_stop();
			}

			CStatManagerContainerThread::getInstance()->StopBackgroundThread();
			log_info("TTCAgent stopped");
			return 0;
		} else if (p > 0) {
			log_info("wait for child ...");
			int status;
			int ret = waitpid(p, &status, 0);
			while (ret < 0 && errno == EINTR) {
				ret = waitpid(p, &status, 0);
			}

			if (ret < 0) {
				log_error("waitpid error: %m");
				return -1;
			}

			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == 0)
					return 0;
				else if ((char) WEXITSTATUS(status) < 0) {
					//瀛愯繘绋媍rash鎺夊悗锛�鍚戣繍缁村钩鍙扮煭淇″憡璀�
					log_error("agent parent cash down 1: %d",
							WEXITSTATUS(status));
					AgentAlert::GetInstance().AlarmToPlatform(
							(char) WEXITSTATUS(status), 1, "", PROCESS_ALARM);
					return WEXITSTATUS(status);
				}
				//瀛愯繘绋媍rash鎺夊悗锛�鍚戣繍缁村钩鍙扮煭淇″憡璀�
				log_error("agent parent cash down 2: %d", WEXITSTATUS(status));
				AgentAlert::GetInstance().AlarmToPlatform(WEXITSTATUS(status),
						1, "", PROCESS_ALARM);
				//just restart	
			} else if (WIFSIGNALED(status)) {
				//瀛愯繘绋媍rash鎺夊悗锛�鍚戣繍缁村钩鍙扮煭淇″憡璀�
				AgentAlert::GetInstance().AlarmToPlatform(WTERMSIG(status), 2,
						"", PROCESS_ALARM);
				log_error("killed by signal %d", WTERMSIG(status));
				//restart
			}
		} else {
			log_error("fork error, %m");
			return -1;
		}
	}
}

