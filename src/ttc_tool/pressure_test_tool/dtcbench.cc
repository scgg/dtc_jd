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

void ShowUsage()
{
   
    printf("		 -t tablename\n");
    printf("		 the tablename to  test\n");
	printf("		 -k key_value\n");
    printf("		 the key to test\n");
    printf("		 -o operation\n");
    printf("		 0:get , 1: insert; 2: update ,default get;\n");
    printf("		 -i dtc or agent ip\n");
    printf("		 dtc or agent ip\n");
    printf("		 -p dtc  or agent port \n");
    printf("		 dtc or agent port\n");  
    printf("		 -a access_token\n");
    printf("		 dtc's accss_token\n");	
	printf("	 	 -c concurrency\n");
    printf("		 Number of multiple requests\n");
	printf("		 -n request numbers\n");
    printf("	 	 Number of requests to perform\n");
	printf("		 -l api log leve(defaul 0)\n");
    printf("	 	 api log leve(defaul 0)\n");
	printf("		 -s compress level\n");
    printf("	 	 compress level(defaul 0)\n");
	printf("		 -m time out \n");
    printf("	 	 time out(the time out of one request (sec), default 5)\n");
	printf("		 -r key operation type(default 0 )\n");
    printf("	 	 0: fixed key, 1: incremental key\n");
	printf("		 -h help\n");
    printf("	 	 for help\n");
    return;
}
/*入参部分全局变量*/
char g_szTableName[1024];
char g_szKeyValue[1024];
char g_szIP[1024];
char g_szPort[1024];
char g_szAccessKey[1024];
char g_szConcurrency[1024];
char g_szReqNum[1024];
char g_szOpType[1024];
char g_szLevel[1024];
char g_szCompressLevel[1024];
char g_szTimeOut[1024];
char g_szKeyOp[1024];
int iAPILogLevel = 0;
int iConcurrency;
int iReqNum;
int ikey;
int iOpType = 0;
int iCompressLevel = 0;
int iTimeOut = 5;
int iKeyOp = 0;
void InitArgv()
{
	ikey = 0;
	iConcurrency = 0;
	iReqNum = 0;
	iOpType = 0;	
	iKeyOp = 0;
	memset(g_szKeyValue, 0 , sizeof(g_szKeyValue));
	memset(g_szIP, 0 , sizeof(g_szIP));
	memset(g_szPort, 0 , sizeof(g_szPort));
	memset(g_szAccessKey, 0 , sizeof(g_szAccessKey));
	memset(g_szConcurrency, 0 , sizeof(g_szConcurrency));
	memset(g_szReqNum, 0 , sizeof(g_szReqNum));	
	memset(g_szTableName, 0 , sizeof(g_szTableName));
	memset(g_szOpType, 0 , sizeof(g_szOpType));
	memset(g_szLevel, 0 , sizeof(g_szLevel));
	memset(g_szCompressLevel, 0 , sizeof(g_szCompressLevel));
	memset(g_szTimeOut, 0 , sizeof(g_szTimeOut));
	memset(g_szKeyOp, 0 , sizeof(g_szKeyOp));

}


int  ParseArgv(int argc, char *argv[])
{
	
	int iOpt;
	while((iOpt = getopt(argc, argv, "t:k:o:i:p:a:c:n:h:s:l:m:r:")) != -1)
	{
		
		switch(iOpt)
		{
			case 't':
				STRCPY(g_szTableName, optarg);				
				break;
				
			case 'k':
				STRCPY(g_szKeyValue, optarg);
				ikey = atoi(g_szKeyValue);
				break;
				
			case 'o':
				STRCPY(g_szOpType, optarg);
				iOpType = atoi(g_szOpType);
				break;
				
			case 'i':
				STRCPY(g_szIP, optarg);				
				break;

			case 'p':
				STRCPY(g_szPort, optarg);				
				break;

			case 'a':
				STRCPY(g_szAccessKey, optarg);				
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
			case 's':
				STRCPY(g_szCompressLevel, optarg);
				iCompressLevel = atoi(g_szCompressLevel);
				break;
			case 'm':
				STRCPY(g_szTimeOut, optarg);
				iTimeOut = atoi(g_szTimeOut);
				break;
			case 'r':
				STRCPY(g_szKeyOp, optarg);
				iKeyOp = atoi(g_szKeyOp);
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
void PrintArgv()
{
	printf("argv: key- %d, concurrency - %d, ReqNum - %d, optype - %d, accesskey - %s,  ip - %s,  port - %s,  tablename - %s, compress level  -%d, timeout %d sec , key operation type %d\n",
			ikey , iConcurrency, iReqNum, iOpType, g_szAccessKey, g_szIP ,g_szPort,  g_szTableName, iCompressLevel, iTimeOut, iKeyOp);
}

void StartDtcPressureTest()
{
	DTC_PRESSURE_CTRL->start(iConcurrency, iReqNum, 0);
}

void WaitBeforeExit(int iSec)
{
		DTC_PRESSURE_CTRL->wait(iSec);
}

void PrintPressureResult()
{
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
	if (iTotalSec > EPSINON)
	{
		printf("平均每秒请求次数： %d reqs/s  \n" , int( DTC_PRESSURE_CTRL->getRequests() / iTotalSec));
	}
	
	
}
void DtcPressure_Join_In()
{
	DTC_PRESSURE_CTRL->join_in();
}
int main(int argc, char *argv[])
{
	if (argc <= 1)
	{
		ShowUsage();
		return 0;
	}
	InitArgv();
	if (0 != ParseArgv(argc, argv))
	{
		 printf("ParseArgv fail \n");
		 return 1;
	}
	PrintArgv();
	SetFdLimit(10240);
	
	if (0 == iConcurrency
        || 0 == 	iReqNum)
	{
		 printf("the argument of (-c or -n ) is zero\n");
		 return 1;
	}
	StartDtcPressureTest();
	DtcPressure_Join_In();
	 
	PrintPressureResult();
	
	return 0;
}
