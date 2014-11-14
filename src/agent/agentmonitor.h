/*
 * agentmonitor.h
 *
 *  Created on: 2014Äê11ÔÂ13ÈÕ
 *      Author: Jiansong
 */

#ifndef AGENTMONITOR_H_
#define AGENTMONITOR_H_

#include<string.h>
#include<stdio.h>
#include<curl_http.h>
#include"json/json.h"
#include "log.h"
#include "ttcapi.h"
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <map>
#include<stdint.h>
#include "unix_lock.h"
#include "mysql.h"
#include "errmsg.h"
#include"admin.h"
#include "admin_protocol.pb.h"
#include "sockaddr.h"
#include "sock_util.h"
#include <errno.h>
#include <sstream>

typedef struct st_mysql MYSQL;


struct compensiteitem
{
	int agentId;
	int state;
	uint64_t idmpt;
};
struct agentstatus
{
	int agentid;
	/*0 for init
	  1 for run
	  2 for error */
	int status;
};

int AgentPing(int agentid, std::string agentAddress);
int Connect();
int loadconfig();
int writebackcompensiteitems() ;
int sndcompensitelist() ;
int pncompensitelist();
int loadagentstat();
int pnagentststat();
int writeagentstat();
int ReportToAdmin(int agentid, int state, uint64_t idmpt);
void show_usage(const char *prog);
bool ReportToAlarm(std::string ip);


#endif /* AGENTMONITOR_H_ */
