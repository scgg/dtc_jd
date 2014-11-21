/*
 * =====================================================================================
 *
 *       Filename:  purge_tool.cc
 *
 *    Description:  purge key by you set
 *
 *        Version:  1.0
 *        Created:  1/21/2010 09:33:01 PM
 *       Revision:  $Id$
 *       Compiler:  g++
 *
 *         Author:  carolynlong (), carolynlong@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ttcapi.h"
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

//global var

//以下参数为程序的输入参数
#define MAXFAILED 10000
#define VERSION "1.0.0"
#define RELEASE "0002"
static char *g_ttc_adress = NULL;	//ttc地址
static char *g_tablename = NULL;	//表名
static int db_begin = 0;	//尾号开始段
static int db_end = 0;	//尾号结束段
static int div_value = 0;
static int mod_value = 0;
static unsigned int max_failed_num = MAXFAILED; 
static char *keylist_file = NULL;//keylist 文件的文件路径
static bool quiet = false;

//可选参数

#define V_OPT(argc, argv)	do {\
	if(!(optarg && optarg[0])) {\
		ShowUsage(argv[0]);\
		return -1;\
	}\
}while(0)


int Purge(unsigned int key, TTC::Server *s)
{

	TTC::Result *t;
	TTC::PurgeRequest r(s);
	r.SetKey(key);
	t = r.Execute();
	if(t->ResultCode())
	{
		printf("purge err: key=%u, rc=%d\n", key, t->ResultCode());
		return -1;
	}

	delete t;
	return 0; 
}

static void ShowUsage(char *s)
{
	printf(	"usage: %s -a ttc_adress -t tablename -b key_begin -e key_end -m mod -d div -f keylist_file"
			" [-s] [-n max_failed_num]\n"
			"	-a ttc_address\n"
			"		ttc's address. ip:port/protocol\n"
			"	-t tablename\n"
			"		ttc's tablename\n"
			"	-b key_begin\n"
			"		the first key to be purge, must > 0\n"
			"	-e key_end\n"
			"		the last key to be purge must >= tail_begin\n"
			"	-m mod\n"
			"		mod value to distribute key to db\n"
			"	-d div\n"
			"		div value to distribute key to db\n"   
			"	-f keylist_file\n"
			"		get keys from keylist file\n"
			"	-n max_failed_num\n"
			"		if the faild key num is more then this value ,processing be endedi\n"
			"	-s keep silence\n"
			"		if not set this parameter,it need make sure to start purge\n",s);
	exit(0);
}


/* parse user input arguments */
static int parse_argv(int argc, char **argv)
{
	int iOpt = 0;
	int maxfailed = 0;

	while((iOpt = getopt(argc, argv, "a:t:b:e:m:d:f:T:n:sv")) != -1)
	{
		switch(iOpt)
		{
			case -1:
			case 0:
				break;
			case 'a':
				V_OPT(argc, argv);
				g_ttc_adress = optarg;
				break;

			case 't':
				V_OPT(argc, argv);
				g_tablename = optarg;
				break;

			case 'b':
				V_OPT(argc, argv);
				db_begin = atoi(optarg);
				break;

			case 'e':
				V_OPT(argc, argv);
				db_end = atoi(optarg);
				break;

			case 'm':
				V_OPT(argc, argv);
				mod_value = atoi(optarg);
				break;

			case 'd':
				V_OPT(argc, argv);
				div_value = atoi(optarg);
				break;

			case 'f':
				V_OPT(argc, argv);
				keylist_file = optarg;
				break;
			case 's':
				quiet = true;
				break;
			case 'n':
				maxfailed = atoi(optarg);
				break;
			case 'v':
				printf("purge_tool:%s_release_%s\n",VERSION,RELEASE);
				exit(0);
			default:
				fprintf(stderr, "invalid option: %c\n", iOpt);
				ShowUsage(argv[0]);
		}		
	}

	if(argc < 8)
	{
		
		ShowUsage(argv[0]);
	}


	//校验输入参数的合法性

	if (db_begin < 0 || db_begin > 999)
	{
		fprintf(stderr,"db_begein must between 0-999\n");
		ShowUsage(argv[0]);
	}

	if (db_end < 0 || db_end > 999)
	{
		fprintf(stderr,"db_end must between 0-999\n");
		ShowUsage(argv[0]);
	}
	else if (db_end<db_begin)
	{
		fprintf(stderr,"db_end must >= db_begin\n");
		ShowUsage(argv[0]);
	}

	if(maxfailed < 0)
	{
		fprintf(stderr," max failed num is invalid \n");
		ShowUsage(argv[0]);
	}
	else if(maxfailed !=0) 
	{
		max_failed_num = maxfailed;
	
	}
	else
	{
		fprintf(stderr,"use default max failed num :%d\n",MAXFAILED);
	}
	
	if(div_value <=0)
	{
		fprintf(stderr,"div value is invalid\n");
		ShowUsage(argv[0]);
	}

	if(mod_value <= 0)
	{
		fprintf(stderr,"mod value is invalid\n");
		ShowUsage(argv[0]);
	}
	//get keys from keylist file
	if (keylist_file == NULL)
	{
		printf("please input key_list file!\n\n");
		ShowUsage(argv[0]);
	}

	return 0;
}

int main(int argc, char **argv)
{
	if(parse_argv(argc, argv)) 
	{
		return -1;
	}

	TTC::Server server;
	server.SetAddress(g_ttc_adress);
	server.SetTableName(g_tablename);
	server.SetMTimeout(1000);
	server.IntKey();
	if(server.Ping()!= 0)
	{
		printf("Connect TTC [%s] failed,please be sure ttc have started\n",g_ttc_adress);
		exit(0);
	}

	if(quiet == false)
	{
		printf("Are you sure to purge db[%d-%d] from ttc [%s](y/n)",db_begin,db_end,g_ttc_adress);

		while (1)
		{
			char temp[512] = "";
			scanf("%s",temp);
			if (temp[0] == 'n' || temp[0] == 'N')
				_exit(-1);

			if (temp[0] == 'y'||temp[0] == 'Y')
				break;

			printf("Are you sure to purge db[%d-%d] from ttc [%s](y/n)",db_begin,db_end,g_ttc_adress);
		}
	}

	ifstream fin;
	fin.open(keylist_file);
	int key_temp;
	FILE *fp = fopen("purge_faild_key.log","w+"); 

	if(fp == NULL)
	{
		printf("open purge_faild_key.log error :%m\n");
		return -1;
	}

	if (!fin)
	{   
		printf("open keylist file:%s failed:%m\n\n",keylist_file);
		fclose(fp);
		return -1; 
	}   
	printf("start purge.....get keys from keylist file\n");

	int dbix = 0;
	unsigned int total_num = 0;
	unsigned int failed_num = 0;
	struct timeval tv1;
	struct timeval tv2;
	gettimeofday(&tv1,NULL);

	while(fin>>key_temp)
	{
		dbix = key_temp/div_value%mod_value;
		if(db_begin <= dbix && dbix<= db_end)
		{
			int iret = Purge(key_temp,&server);
			if(iret)
			{
				fprintf(fp,"%d\n",key_temp);
				failed_num++;
				if(failed_num >= max_failed_num)//failed num is more than given value;
				{
					printf("Purge failed!\n");
					printf("Current  failed key is more than %d!\n",max_failed_num);
					fclose(fp);
					fin.close();
					exit(-1);
				}
				
			}
			total_num++;
		}
	}
	
	fclose(fp);
	fin.close();
	
	gettimeofday(&tv2,NULL);
	uint64_t v = (uint64_t)tv2.tv_sec *1000000 + tv2.tv_usec;
	v -= ((uint64_t)tv1.tv_sec*1000000 + tv1.tv_usec);

	printf("purge done!\n");
	printf("statistic info:\n");
	printf("\tcost time: sec:%u\t usec:%u\n",(unsigned int)(v/1000000),(unsigned int)(v%1000000));
	printf("\ttotal purge key num :%u\n",total_num);
	printf("\tfailed purge key num :%u\n",failed_num);

	return 0;
}
