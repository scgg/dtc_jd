/*
 * dtcinstancemonitor.h
 *
 *  Created on: 2014Äê10ÔÂ22ÈÕ
 *      Author: qiujiansong
 */

#ifndef DTCINSTANCEMONITOR_H_
#define DTCINSTANCEMONITOR_H_
#include<string.h>
#include<stdio.h>
#include<tinyxml.h>
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

typedef struct st_mysql MYSQL;


struct compensiteitem
{
	int busId;
	char addr[15] ;
	int state;
	char insId[10];
	uint64_t idmpt;
};

struct instancestatus
{
	int busId;
	char insId[10];
	/*0 for init
	  1 for run
	  2 for error */
	int status;
};


int Connect();
int ReportToAdmin(int busId, char addr[], int state, uint64_t idmpt);
bool ReportToAlarm(std::string ip);
int loadconfig();
int loadinststat();
int sndcompensitelist();
int DtcPing(std::string dtcAddress);
int writeinststat();
int writebackcompensiteitems();
void show_usage(const char *prog);
int pncompensitelist();
int pninststat();

#endif /* DTCINSTANCEMONITOR_H_ */
