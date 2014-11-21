/*
 * =====================================================================================
 *
 *       Filename:  dtchttpd.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  28/07/2014 20:00:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  samuelzhang
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
#include "pod.h"
#include "LockFreeQueue.h"
#include <sstream>
#include "fk_config.h"
#include "dtchttpd_version.h"
#include "dtchttpd_alarm.h"
#include "daemon.h"
#include "sys/sysinfo.h"
#include "join.h"
#include "fd_transfer.h"
#include "helper.h"
#include <ext/hash_set>
#include "storage_manager.h"
#include "config.h"

//global variables
FK_Config g_config;
LockFreeQueue<MSG> g_Queue;
dtchttpd::CConfig g_storage_config;
//cascade settings
std::string g_another_dtchttpd_url;
__gnu_cxx::hash_set<int> g_gray_bids;

int ListenOn(const std::string &addr);

int main(int argc, char ** argv)
{
	//get command parameters
	if (argc < 2)
	{
		ShowUsage();
		exit(-1);
	}
	char listen_port[256];
	char db_conf[256];
	char log_switch[256];
	int c;
	while ((c = getopt(argc, argv, "p:c:d:hv")) != -1)
	{
		switch (c)
		{
			case 'p':
				strncpy(listen_port, optarg, sizeof(listen_port) - 1);
				break;
			case 'c':
				strncpy(db_conf, optarg, sizeof(db_conf) - 1);
				break;
			case 'd':
				strncpy(log_switch, optarg, sizeof(log_switch) - 1);
				break;
			case 'h':
				ShowUsage();
				exit(0);
			case 'v':
				ShowVersion();
				exit(0);
			case '?':
            default :
				ShowUsage();
				exit(-1);
		}
	}
	//set conf file
	char config_error_msg[256];
	if (g_config.Init(db_conf, config_error_msg) < 0)
	{
		printf("init %s error message: %s", db_conf, config_error_msg);
		return -1;
	}
	if (g_storage_config.ParseConfig() < 0)
	{
		printf("CConfig parse config failed");
		return -1;
	}
	dtchttpd::Alarm::GetInstance().SetConf(db_conf);   
	//make program run in daemon
	//signal(SIGPIPE,SIG_IGN);
	daemon(1, 0);
	//set log directory
	_init_log_("dtchttpd", "./log");
	//main logic
	while(true)
	{
		pid_t p = fork();
		if(p == 0)
		{
			log_info("dtchttpd start...");
			//set log level
			if (1 != atoi(log_switch))
				_set_log_level_(3);
			//set signal action
			daemon_start();
			//set process open file limits
			DaemonSetFdLimit(100000);
			//get cascade dtchttpd and ping it
			g_config.GetStringValue("cascade_url", g_another_dtchttpd_url);
			if (g_another_dtchttpd_url.empty())
			{
				log_info("not set cascade settings");
			}
			else
			{
				log_info("set cascade settings, url is %s", g_another_dtchttpd_url.c_str());
				//get gray bids
				std::string gray_bids;
				g_config.GetStringValue("gray_bid", gray_bids);
				if (false == gray_bids.empty())
				{
					g_gray_bids = SplitString(gray_bids);
					//ping to make sure cascade dtchttpd alive
					std::string ping_content = "{\"curve\":1001}";
					int ping_ret = DoHttpRequest(g_another_dtchttpd_url, ping_content);
					if (0 != ping_ret)
					{
						log_error("ping cascade dtchttpd failed");
						return 0;
					}
				}
				else
				{
					log_error("set cascade url but not set gray bids");
				}
			}
			//worker thraeds : (1)recv (2)parse
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
			if (listenfd == -1)
			{
				log_error("listen on %s port failed", listen_port);
				return 0;
			}
			//join threads do accept fd job 
			Join join_thread1("join1", listenfd, fdGroup1);
			join_thread1.InitializeThread();
			join_thread1.RunningThread();
			//storage layer: write data to mysql
			dtchttpd::CStorageManager storage_manager(g_storage_config);
			storage_manager.Run();
			//init alarm
			dtchttpd::Alarm::GetInstance().Init(core_num);
			//scan to alarm
			while(httpdRunning)
			{
				sleep(10);
				if (!httpdRunning)
					break;
				
				//lock free queue size alarm
				if (g_Queue.Size()*2 > g_Queue.QueueSize())
				{
					log_error("queue size is %u has use %u", g_Queue.QueueSize(), g_Queue.Size());
					dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::QUEUE_CONGEST, g_Queue.QueueSize(), g_Queue.Size());
				}
				//thread cpu rate alarm
				if (true == dtchttpd::Alarm::GetInstance().ScanCpuThreadStat())
				{
					log_error("dtchttpd cpu use rate too high");
					dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::CPU_RATE_HIGH);
				}
			}
			log_info("dtchttpd stopped");
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
			if (ret < 0)
			{
				log_error("waitpid error");
				dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::WAIT_PID_FAIL);
				return -1;
			}
			if (WIFEXITED(status))
			{
				if(WEXITSTATUS(status) == 0)
					return 0;
				else if((char)WEXITSTATUS(status) < 0)
					return WEXITSTATUS(status);
			}
			else if(WIFSIGNALED(status))
			{
				log_error("killed by signal %d", WTERMSIG(status));
				dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::KILL_BY_SIGNAL, WTERMSIG(status));
			}
		}
		else
		{
			log_error("fork error");
			dtchttpd::Alarm::GetInstance().ReportToPlatform(dtchttpd::FORK_ERROR);
			return -1;
		}
	}
}

int ListenOn(const std::string &addr)
{
	CSocketAddress bindAddr;
	const char *errmsg = NULL;
	errmsg = bindAddr.SetAddress(addr.c_str(), (const char *)NULL);
	if(errmsg)
	{
		log_error("%s %s", errmsg, addr.c_str());
		return -1;
	}

	int netfd = bindAddr.CreateSocket();
	if(netfd < 0)
	{
		log_error("acceptor create socket failed: %m, %d", errno);
		return -1;
	}

	int optval = 1;
	setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
	optval = 60;
	setsockopt(netfd, SOL_TCP, TCP_DEFER_ACCEPT, &optval, sizeof(optval));

	if(bindAddr.BindSocket(netfd) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor bind address failed: %m, %d", errno);
		return -1;
	}

	if(bindAddr.SocketType() == SOCK_STREAM && listen(netfd, 20000) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor listen failed: %m, %d", errno);
		return -1;
	}

	return netfd;
}
