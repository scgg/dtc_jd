#include <stdio.h>
#include <iostream>
#include<unistd.h>
#include <string.h>
#include <stdlib.h>
#include "dtcpressure.h"
#include <sys/resource.h>
#define STRCPY(d,s) do{ strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; }while(0)
const float EPSINON = 0.00000001;

int SetFdLimit(int maxfd)
{
	struct rlimit rlim;
	if(maxfd)
	{
		/* raise open files */
		rlim.rlim_cur = maxfd;
		rlim.rlim_max = maxfd;
		if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
			printf("Increase FdLimit failed, set val[%d] errmsg[%s]",maxfd,strerror(errno));
			return -1;
		}
	}
	return 0;
}

void ShowUsage(){
	printf("		 -k key_value, the key to test\n");
    printf("		 -o operation, 0:get , 1: insert, 2: update, 3: delete; default get;\n");
	printf("	 	 -c concurrency, Number of multiple requests\n");
	printf("		 -n request numbers,  Number of requests to perform\n");
	printf("		 -l api log leve(defaul 0)\n");
	printf("		 -h help\n");
    printf("	 	 for help\n");
    return;
}
/*入参部分全局变量*/
char g_szKeyValue[1024];
char g_szConcurrency[1024];
char g_szReqNum[1024];
char g_szOpType[1024];
char g_szLevel[1024];
int iAPILogLevel = 0;
int iConcurrency;
int iReqNum;
int ikey;
int iOpType = 0;
void InitArgv(){
	ikey = 0;
	iConcurrency = 0;
	iReqNum = 0;
	memset(g_szKeyValue, 0 , sizeof(g_szKeyValue));
	memset(g_szConcurrency, 0 , sizeof(g_szConcurrency));
	memset(g_szReqNum, 0 , sizeof(g_szReqNum));	
	memset(g_szOpType, 0 , sizeof(g_szOpType));
	memset(g_szLevel, 0 , sizeof(g_szLevel));
}


int  ParseArgv(int argc, char *argv[]){
	int iOpt;
	while((iOpt = getopt(argc, argv, "k:o:c:n:l:h:")) != -1){
		switch(iOpt){
		case 'k':
			STRCPY(g_szKeyValue, optarg);
			ikey = atoi(g_szKeyValue);
			break;
		case 'o':
			STRCPY(g_szOpType, optarg);
			iOpType = atoi(g_szOpType);
			break;
		case 'c':
			STRCPY(g_szConcurrency, optarg);
			iConcurrency = atoi(g_szConcurrency);
			break;
		case 'n':
			STRCPY(g_szReqNum, optarg);
			iReqNum = atoi(g_szReqNum);
			break;
		case 'l':
			STRCPY(g_szLevel, optarg);
			iAPILogLevel = atoi(g_szLevel);
			break;
		case 'h':
			ShowUsage();
			break;
		default:
			fprintf(stderr, "invalid option: %c\n", iOpt);
			ShowUsage();
			return -1;
		}
    }
	return 0;
}
void PrintArgv(){
	printf("argv: key- %d, optype - %d, concurrency - %d, ReqNum - %d\n",
			ikey , iOpType, iConcurrency, iReqNum);
}

void StartDtcPressureTest(){
	DTC_PRESSURE_CTRL->start(iConcurrency, iReqNum, 0);
}

void WaitBeforeExit(int iSec){
		DTC_PRESSURE_CTRL->wait(iSec);
}

void PrintPressureResult(){
	printf("测试接口名称: %s \n 并发线程数: %u \n 请求总数: %u \n 请求处理总耗时: %u us \n "
		   "最大请求处理耗时:%u us  \n 最小请求处理耗时:%u us \n  请求失败次数:%u \n", 
			DTC_PRESSURE_CTRL->getName().c_str(),
			iConcurrency,
			DTC_PRESSURE_CTRL->getRequests(),
			DTC_PRESSURE_CTRL->getTotalElapse(),				
			DTC_PRESSURE_CTRL->getMaxElapse(),
			DTC_PRESSURE_CTRL->getMinElapse(),
			DTC_PRESSURE_CTRL->getFailRequests()
			);
    float iTotalSec = (float)DTC_PRESSURE_CTRL->getTotalElapse() / 1000000;
	if (iTotalSec > EPSINON){
		printf("平均每秒请求次数： %d reqs/s  \n" , int( DTC_PRESSURE_CTRL->getRequests() / iTotalSec));
	}
	
	
}
void DtcPressure_Join_In(){
	DTC_PRESSURE_CTRL->join_in();
}
int main(int argc, char *argv[]){
	if (argc <= 1){
		ShowUsage();
		return 0;
	}
	InitArgv();
	if (0 != ParseArgv(argc, argv)){
		 printf("ParseArgv fail \n");
		 return 1;
	}
	PrintArgv();
	SetFdLimit(10240);
	
	if (0 == iConcurrency || 0 == 	iReqNum){
		 printf("the argument of (-c or -n ) is zero\n");
		 return 1;
	}
	StartDtcPressureTest();
	DtcPressure_Join_In();
	PrintPressureResult();
	
	return 0;
}
