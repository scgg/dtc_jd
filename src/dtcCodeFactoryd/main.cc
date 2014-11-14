/*
 * =====================================================================================
 *
 *       Filename:  main.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  28/10/2014 11:00:00 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  adamxu
 *
 * =====================================================================================
 */

#include <stdint.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include "poll_thread_group.h"
#include "log.h"
#include "mysql_manager.h"
#include <sstream>
#include "fk_config.h"
#include "dtcCodeFactoryd_version.h"
#include "dtcCodeFactoryd_alarm.h"
#include "daemon.h"
#include "sys/sysinfo.h"
#include "join.h"
#include "fd_transfer.h"

//global variables
MySqlManager *g_manager;
std::string g_strGenPath;
std::string g_versioninfo;

int ListenOn(const std::string &addr);

int main(int argc, char ** argv){
	//get command parameters
	int ret = 0;
	if (argc < 2){
		ShowUsage();
		exit(-1);
	}
	char listen_port[256];
	char db_conf[256];
	char log_switch[256];
	int c = -1;
	g_versioninfo = GetCurrentVersion();
	while ((c = getopt(argc, argv, "h:v:p:c:d:")) != -1){
		switch (c){
		case 'h':
			ShowUsage();
			exit(0);
		case 'v':
			printf("dtcCodeFactoryd version: %s\n", g_versioninfo.c_str());
			break;
		case 'p':
			strncpy(listen_port, optarg, sizeof(listen_port) - 1);
			break;
		case 'c':
			strncpy(db_conf, optarg, sizeof(db_conf) - 1);
			break;
		case 'd':
			strncpy(log_switch, optarg, sizeof(log_switch) - 1);
			break;
		case '?':
		default :
			ShowUsage();
			exit(-1);
		}
	}
	//set conf file
	FK_Config config;
	char config_error_msg[256];
	if (config.Init(db_conf, config_error_msg) < 0){
		printf("init %s error message: %s", db_conf, config_error_msg);
		return -1;
	}
	dtcCodeFactoryd::Alarm::GetInstance().SetConf(db_conf);
	//make program run in daemon
	//signal(SIGPIPE,SIG_IGN);
	daemon(1, 0);
	//main logic
	while(true){
		pid_t p = fork();
		if(p == 0){
			log_info("codeFactoryd start...");
			//set log dir and level
			_init_log_("codeFactoryd", "./log");
			if (1 == atoi(log_switch))
				_set_log_level_(7);
			else
				_set_log_level_(3);
			//set signal action
			daemon_start();
			//set process open file limits
			DaemonSetFdLimit(102400);
			//use MySqlManager to write mysql database
			std::string db_ip,db_user,db_pass,db_name;
			int db_port;
			config.GetStringValue("gen_path", g_strGenPath);
			if (g_strGenPath == ""){
				log_error("get gen_path empty! please check dtcCodeFactoryd.conf!");
				return -1;
			}
			config.GetStringValue("db_ip", db_ip);
			config.GetStringValue("db_user", db_user);
			config.GetStringValue("db_pass", db_pass);
			config.GetIntValue("db_port", db_port);
			config.GetStringValue("db_name", db_name);
			string errMsg;
			g_manager = new MySqlManager(db_ip, db_user, db_pass, db_port, db_name);
			ret = g_manager->GetConnection(errMsg);
			if(0 != ret){
				log_error("main: CodeFactory server get mysql connection fail, errCode: %d, errMsg: %s", ret, errMsg.c_str());
			}
			//worker thraeds : (1)recv (2)parse (3)merge
			int core_num = get_nprocs() > 1 ? get_nprocs() : 1; //cpu core number
			log_info("the cpu core number of this machine is %d", core_num);
			CPollThreadGroup *pollThreadGroup = new CPollThreadGroup("worker");
			pollThreadGroup->Start(core_num, 100000);
			//fd transfer
			CFDTransferGroup *fdGroup1 = new CFDTransferGroup(core_num);
			fdGroup1->Attach(pollThreadGroup);
			pollThreadGroup->RunningThreads();
			//get listen fd
			int listenfd = ListenOn("*:" + std::string(listen_port));
			if (listenfd == -1){
				log_error("listen on %s port failed", listen_port);
				return 0;
			}
			//two join threads do accept fd job 
			Join join_thread1("join1", listenfd, fdGroup1);
			join_thread1.InitializeThread();
			join_thread1.RunningThread();
			//init alarm
			dtcCodeFactoryd::Alarm::GetInstance().Init(core_num);
			//httpd stopped
			while(agentRunning)
				pause();
			log_info("codeFactoryd stopped");
			return 0;
		}else if (p > 0){
			log_info("wait for child ...");
			int status;
			int ret = waitpid(p, &status, 0);
			while(ret < 0 && errno == EINTR){
				ret = waitpid(p, &status, 0);
			}
			if (ret < 0){
				log_error("waitpid error");
				dtcCodeFactoryd::Alarm::GetInstance().ReportToPlatform(dtcCodeFactoryd::WAIT_PID_FAIL);
				return -1;
			}
			if (WIFEXITED(status)){
				if(WEXITSTATUS(status) == 0)
					return 0;
				else if((char)WEXITSTATUS(status) < 0)
					return WEXITSTATUS(status);
			}else if(WIFSIGNALED(status)){
				log_error("killed by signal %d", WTERMSIG(status));
				dtcCodeFactoryd::Alarm::GetInstance().ReportToPlatform(dtcCodeFactoryd::KILL_BY_SIGNAL, WTERMSIG(status));
			}
		}else{
			log_error("fork error");
			dtcCodeFactoryd::Alarm::GetInstance().ReportToPlatform(dtcCodeFactoryd::FORK_ERROR);
			return -1;
		}
	}
}

int ListenOn(const std::string &addr){
	CSocketAddress bindAddr;
	const char *errmsg = NULL;
	errmsg = bindAddr.SetAddress(addr.c_str(), (const char *)NULL);
	if(errmsg){
		log_error("%s %s", errmsg, addr.c_str());
		return -1;
	}

	int netfd = bindAddr.CreateSocket();
	if(netfd < 0){
		log_error("acceptor create socket failed: %m, %d", errno);
		return -1;
	}

	int optval = 1;
	setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
	optval = 60;
	setsockopt(netfd, SOL_TCP, TCP_DEFER_ACCEPT, &optval, sizeof(optval));

	if(bindAddr.BindSocket(netfd) < 0){
		close(netfd);
		netfd = -1;
		log_error("acceptor bind address failed: %m, %d", errno);
		return -1;
	}

	if(bindAddr.SocketType() == SOCK_STREAM && listen(netfd, 20000) < 0){
		close(netfd);
		netfd = -1;
		log_error("acceptor listen failed: %m, %d", errno);
		return -1;
	}

	return netfd;
}
