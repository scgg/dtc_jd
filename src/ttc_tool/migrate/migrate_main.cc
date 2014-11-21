/*
 * =====================================================================================
 *
 *       Filename:  migrate_main.cc
 *
 *    Description:  迁移工具
 *
 *        Version:  1.0
 *        Created:  11/11/2010 02:31:39 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <stdio.h>
#include "migrate_main.h"
const char *version = "0.1";

sockaddr_in listen_on;
int log_level = 6;
int bind_fd = -1;

void show_usage(const char *prog)
{
	printf("%s -l [ip:]port(port 9495 is recommend) [-L loglevel(1-7)]\n", prog);
}

void parse_args(int argc, char **argv)
{
	int c = -1;
	std::string bind_addr = "";

	while((c = getopt(argc, argv, "l:L:v")) != -1)
	{
		switch(c)
		{
			case 'l':
				bind_addr = optarg;
				break;
			case 'L':
				log_level = atoi(optarg);
				break;
			case '?':
				show_usage(argv[0]);
				exit(1);
			case 'v':
				printf("migrate_agnet %s %s\n", version, REVISION);
				exit(0);
		}
	}

	if (bind_addr == "")
	{
		show_usage(argv[0]);
		exit(1);
	}

	log_level = log_level<1 ? 1 : (log_level>7 ? 7:log_level);
	listen_on = string_to_sockaddr(bind_addr);
	fprintf(stderr,"%s starting.\nbind address:%s log level:%d\n",argv[0],bind_addr.c_str(),log_level);
}

int init_server(void)
{
	_init_log_("migrate");
	_set_log_level_(log_level);
	bind_fd = socket(PF_INET, SOCK_STREAM, 0);
	if(bind_fd < 0)
	{
		log_error("create bind socket error:%m");
		return -1;
	}
	int tmp = 1;
	setsockopt(bind_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
	int rtn = bind(bind_fd, (sockaddr *)&listen_on, sizeof(sockaddr_in));
	if(rtn < 0)
	{
		log_error("bind error:%m");
		return -1;
	}

	rtn = listen(bind_fd, 1024);
	if(rtn < 0)
	{
		perror("listen error:%m");
		return -1;
	}

	daemon(1, 0);

	mkdir(HISTORY_PATH, 0777);

	if(access(HISTORY_PATH, W_OK|X_OK) < 0)
	{
		log_error("dir(%s) Not writable", HISTORY_PATH);
		return -1;
	}
	return 0;
}

void run_server(void)
{
	while (1)
	{
		sockaddr_in addr = { sizeof(addr) };
		socklen_t socklen = sizeof(addr);
		int fd = accept(bind_fd, (sockaddr *)&addr, &socklen);
		if(fd < 0)
		{
			log_error("accept error, %m bind_fd:%d\n",bind_fd);
			continue;
		}
		int pid = fork();
		if (pid > 0)
		{
				CProcessCommand processUnit(fd);
			processUnit.process();//process this command and close this fd
			return;
		}
		else
				close(fd);
	}
}

void close_server(void)
{
		close(bind_fd);
}

int main(int argc, char **argv)
{
	parse_args(argc,argv);
	if (init_server())
			return -1;
	run_server();
	close_server();
}



