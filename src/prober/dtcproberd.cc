#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include <stdexcept>
#include "version.h"
#include "json/json.h"
#include "poll_thread_group.h"
#include "log.h"
#include "HttpServer.h"
#include "prober_config.h"
#include "ProberCmd.h"
#include "MemoryUsageCmd.h"
#include "memcheck.h"
#include "RelativeHourCalculator.h"
#include "segvcatch.h"

int targetNewHash;
int hashChanging;
CConfig* gConfig = NULL;
CDbConfig* gDbConfig = NULL;
CTableDefinition* gTableDef = NULL;

class CHttpSessionProber : public CHttpSessionLogic
{
public:
	CHttpSessionProber(CProberConfig *proberConfig) {
		gConfig = NULL;
		gDbConfig = NULL;
		gTableDef = NULL;
		m_ProberConfig = proberConfig;
	}
	~CHttpSessionProber() {}
	virtual bool ProcessBody(std::string reqBody, std::string &respBody) {
		Json::Value in, out;
		Json::Reader reader;
		Json::FastWriter writer;
		ProberResultSet prs;
		bool ret = true;

		if (!reader.parse(reqBody, in, false)) {
			prs.retcode = RET_CODE_INTERNAL_ERR;
			prs.errmsg = "解析html请求出错";
			ret = false;
			log_error("parse body error. requst body: %s", reqBody.c_str());
		} else {
			std::string cn = m_ProberConfig->CmdCodeToClassName(in["cmdcode"].asString());
			if (atoi(in["cmdcode"].asString().c_str()) > MEM_CMD_BASE &&
			    !InitMemConf(std::string("/usr/local/dtc/") + in["accesskey"].asString() + "/conf")) {
				prs.retcode = RET_CODE_INTT_MEM_CONF_ERR;
				prs.errmsg = "打开缓存配置出错";
				log_error("init mem conf error, %s", in["accesskey"].asString().c_str());
				ret = false;
			}
			CProberCmd* proberCmd = NULL;
			log_debug("cmd code: %s, class name: %s", in["cmdcode"].asString().c_str(), cn.c_str());
			if (ret) {
				if (std::string("") != cn && (proberCmd = CProberCmd::CreatConcreteCmd(cn)) != NULL) {
					log_debug("proberCmd: %x", proberCmd);
					ret = proberCmd->ProcessCmd(in, prs);
					delete proberCmd;
				} else {
					prs.retcode = RET_CODE_NO_CMD;
					prs.errmsg = "当前版本探针服务不支持此命令";
					ret = false;
					log_error("cmdcode: %s not support", in["cmdcode"].asString().c_str());
				}
			}
		}

		out["retcode"] = prs.retcode;
		out["errmsg"] = prs.errmsg;
		out["resp"] = prs.resp;
		respBody = writer.write(out);

		DELETE(gConfig);
		if (NULL != gDbConfig) {
			gDbConfig->Destroy();
			gDbConfig = NULL;
		}
		DELETE(gTableDef);

		return ret;
	}

	bool InitMemConf(std::string confBase) {
		log_debug("InitMemConf with %s", confBase.c_str());
		std::string cacheCfg = confBase + "/cache.conf";
		std::string tableCfg = confBase + "/table.conf";
		gConfig = new CConfig;
		if(gConfig->ParseConfig(cacheCfg.c_str(), "cache")){
			log_error("parse cache config error");
			return false;
		}
		hashChanging = gConfig->GetIntVal("cache", "HashChanging", 0);
		targetNewHash = gConfig->GetIntVal("cache", "TargetNewHash", 0);
		RELATIVE_HOUR_CALCULATOR->SetBaseHour(gConfig->GetIntVal("cache", "RelativeYear", 2014));
	
		gDbConfig = CDbConfig::Load(tableCfg.c_str());
		if(gDbConfig == NULL){
			log_error("load table configuire file error");
			return false;
		}
	
		gTableDef = gDbConfig->BuildTableDefinition();
		if(gTableDef == NULL){
			log_error("build table definition error");
			return false;
		}
		return true;
	}

private:
	CProberConfig *m_ProberConfig;
};

static char prog_name[] = "dtcproberd";

static volatile int running = 1;

static void sigterm_handler(int signo)
{
	running = 0;
}

static void daemon_start()
{
	struct sigaction sa;
	sigset_t sset;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	signal(SIGPIPE,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);

	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
}

int DaemonSetFdLimit(int maxfd)
{
	struct rlimit rlim;
	if(maxfd)
	{
		/* raise open files */
		rlim.rlim_cur = maxfd;
		rlim.rlim_max = maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
			log_error("Increase FdLimit failed, set val[%d] errmsg[%s]",maxfd,strerror(errno));
			return -1;
		}
	}
	return 0;
}

void ShowUsage(int argc, char **argv) {
	printf("%s [-c conf]\n", argv[0]);
	printf("    -c conf\n");
	printf("        config file, default ../conf/prober.xml\n");
}

void ShowVersion() {
	printf("%s version: %s %s\n", prog_name, version, git_version);
	printf("%s compile date: %s %s\n", prog_name, compdatestr, comptimestr);
}

void handle_segv() {
	throw std::runtime_error("My SEGV");
}

void handle_fpe() {
	throw std::runtime_error("My FPE");
}

int main(int argc, char ** argv)
{
	int c;
	char config_file[256] = "../conf/prober.xml";
	while ((c = getopt(argc, argv, "c:hv")) != -1)
	{
		switch (c)
		{
			case 'c':
				strncpy(config_file, optarg, sizeof(config_file) - 1);
				config_file[sizeof(config_file) - 1] = 0;
				break;
			case 'h':
				ShowUsage(argc, argv);
				exit(0);
			case 'v':
				ShowVersion();
				exit(0);
			case '?':
			default :
				ShowUsage(argc, argv);
				exit(-1);
		}
	}
 
	signal(SIGPIPE,SIG_IGN);
	//no change dir
	daemon(1, 0);

	_init_log_(prog_name, "../log/");
	CProberConfig proberConfig(config_file);
	if (proberConfig.Load()) {
		log_error("Load error");
		return -1;
	}

	segvcatch::init_segv(&handle_segv);
	segvcatch::init_fpe(&handle_fpe);

	while(true)
	{
		pid_t p = fork();
		if(p == 0)
		{
			//if (1 != atoi(log_switch))
			daemon_start();
			DaemonSetFdLimit(102400);
			log_info("%s start...", prog_name);
			//listen on 8080 port
			CPollThreadGroup pollThreadGroup(prog_name);
			pollThreadGroup.Start(1, 100000);
			//CHttpSessionProber *httpSessionProber = new CHttpSessionProber();
			CHttpSessionProber httpSessionProber(&proberConfig);
			CHttpServer httpServer(&pollThreadGroup, &httpSessionProber);
			httpServer.ListenOn(proberConfig.ListenOn());
			pollThreadGroup.RunningThreads();
			while (running)
				pause();
			log_info("%s stopped", prog_name);
			return 0;
		}
		else if (p > 0)
		{
			log_info("wait for child ...");
			int status;
			int ret = waitpid(p, &status, 0);
			while(ret < 0 && errno == EINTR)
			{
				ret = waitpid(p, &status, 0);
			}

			if(ret < 0)
			{
				log_error("waitpid error");
				return -1;
			}

			if(WIFEXITED(status))
			{
				if(WEXITSTATUS(status) == 0)
					return 0;
				else if((char)WEXITSTATUS(status) < 0)
					return WEXITSTATUS(status);
			}
			else if(WIFSIGNALED(status))
			{
				log_error("killed by signal %d", WTERMSIG(status));
			}
		}
		else
		{
			log_error("fork error");
			return -1;
		}
	}
}
