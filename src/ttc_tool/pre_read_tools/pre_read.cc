/*
 * =====================================================================================
 *
 *       Filename:  pre_read.cc
 *
 *    Description:  read all keys you set
 *
 *        Version:  1.0
 *        Created:  08/28/2009 09:33:01 PM
 *       Revision:  $Id$
 *       Compiler:  g++
 *
 *         Author:  foryzhou (), foryzhou@tencent.com
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
#include "mutexwait.h"
#include <fstream>

using namespace std;

#define TAILMASK 1000	//尾数为三位则此值为1000
#define QQ_START 10000	//qq号码开始值
#define QQ_STOP 1600000000	//qq号码结束值


//global var
unsigned int g_start = 0;
unsigned int g_stop = 0; 
static bool g_getkey_from_file = false;//是否从文件读取key，否表示从QQ_START遍历到QQ_STOP

//以下参数为程序的输入参数
static char *g_ttc_adress = NULL;	//ttc地址
static char *g_tablename = NULL;	//表名
static unsigned int g_tail_begin = 0;	//尾号开始段
static unsigned int g_tail_end = 0;	//尾号结束段
static unsigned int g_step = 0;		//每次批量生成的key的最大个数
//可选参数
static unsigned int g_setstart = 0;	//用于指定开始号码，默认从QQ_START开始
static int g_time_mutex = 1;		//限制每次get的时间限制，默认为1ms，即1000次/s	
static char *g_dumpfile = NULL;//keydump 文件的文件路径


/*向server s请求key
  返回值：
0:请求成功
-1:ttc连接失败
-2:请求成功，但是节点为空
 ********/
int Get(unsigned int key, TTC::Server *s)
{
	static mutexwait w;
	//limit the speed of get
	if (g_time_mutex)
	{
		w.msstep(g_time_mutex);
	}
			
	int ret =0;
	int retry=3;
	TTC::Result *t;
	TTC::GetRequest r(s);

	r.SetKey(key);

	//request and retry
	while(1)
	{
		t = r.Execute();
		if(t->ResultCode())
		{
			if (retry == 0)
			{
				printf("ResultCode=%d, From=%s, ERR=%s\n", 
					t->ResultCode(), t->ErrorFrom(), t->ErrorMessage());
				delete t;
				return -1;
			}
			else
			{
				sleep(1);
				retry--;
			}

		}
		else
		{
			break;
		}

	}

	//printf("key=%u, rows=%d\n", key, t->NumRows());
	if(t->NumRows()==0)
	{
		ret = -2;
		//the node is empty node
	}

	delete t;
	return ret;
}


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

int bulkget(unsigned int *key, unsigned int num, TTC::Server *s)
{
	int iret = 0;
	
	//for stat
	unsigned int ok_count = 0;
	unsigned int empty_count = 0;
	unsigned int purgeok_count = 0;
	
	for (unsigned int i=0; i < num; i++)
		{
			iret = Get(key[i], s);	

			if (!iret)
			{
				//right
				ok_count++;
			}
			else if(iret == -1)
			{
				//retry
				printf("%d:%u error\n",i+1,key[i]);

				return iret;
				//connect ttc error
			}
			else if (iret == -2)
			{
				empty_count++;

				if(Purge(key[i],s))
				{
					printf("purge error\n");
				}	
				else
				{
					//			printf("purge ok\n");
					purgeok_count++;
				}
				//purge ok or not
			}
		}
		
		printf(" >read ok:%u\t empty node:%u\t purge ok:%u\n",ok_count, empty_count, purgeok_count);
		ok_count = 0;
		empty_count = 0;
		purgeok_count = 0;
		
		return 0;
}
//处理单个尾号的bulkread
int HandleBulkRead(unsigned int tailnum, TTC::Server *s)
{
	unsigned int* key_temp = NULL;
	

	printf(">%u-%u\tstep:%u tailnum:%u\n",g_start*1000, g_stop*1000,g_step,tailnum);

	unsigned int n_temp = 0;
	int iret = 0;

	for( unsigned int i =g_start;i<g_stop;i+=g_step)
	{
		if(i+g_step<g_stop)
		{
			n_temp = g_step; 
		}
		else
		{
			if (tailnum)
			{
				n_temp = g_stop - i-1;
			}
			else
			{
				n_temp = g_stop - i;
			}
		}

		//generate keys
		key_temp = new unsigned int[n_temp];
		for (unsigned long j=i,k=0;k<n_temp;j++,k++)
		{
			key_temp[k] = j*TAILMASK+tailnum;
		}
		printf(" >%u-%u:%u\t generate ok!\n",i*TAILMASK+tailnum,(i+n_temp)*TAILMASK+tailnum,n_temp);

		//get all by keys
		iret = bulkget(key_temp, n_temp, s);
		if (iret)
		{
			return iret;
		}
		delete[] key_temp;

		key_temp = NULL;
	}
	return 0;
}

//处理批量pre_read, get key from keydump file
int pre_read_use_dumpfile(ifstream & fin, TTC::Server *s)
{
	//key temp
	unsigned int* pkey_temp = NULL;
	unsigned int key_temp = 0;
	unsigned int tailnum = 0;

	//for stat
	unsigned int total_keys = 0;

	unsigned int n_temp = 0;
	int iret = 0;

	while(1)
	{
		pkey_temp = new unsigned int[g_step];

		//generate keys
		while((n_temp<g_step)&&(fin>>key_temp))
		{
			tailnum = key_temp%1000;

			if ((tailnum >= g_tail_begin) && (tailnum <=g_tail_end))
			{
				pkey_temp[n_temp++] = key_temp;
			}

			total_keys++;
		}

		printf(" >%u keys get, %u keys between %u-%u generate ok!\n",total_keys, n_temp, g_tail_begin, g_tail_end);

		//get all by keys
		iret = bulkget(pkey_temp, n_temp, s);
		
		delete[] pkey_temp;
		pkey_temp = NULL;

		if (iret)
		{
			return iret;
		}

		if (n_temp == 0)
		{
			break;
		}
		else
		{
			total_keys = 0;
			n_temp =0;
		}
	}
	return 0;
}

/* usage */
static void ShowUsage(char *s)
{
	printf(	"usage: %s -a ttc_adress -t tablename -b tail_begin -e tail_end -s step [-S num_start] [-T times_per_second] [-F keydump_file]\n"
			"	-a ttc_address\n"
			"		ttc's address. ip:port/protocol\n"
			"	-t tablename\n"
			"		ttc's tablename\n"
			"	-b tail_begin\n"
			"		tail_begin must between 000-999\n"
			"	-e tail_end\n"
			"		tail_end must between 000-999 and >= tail_begin\n"
			"	-s step\n"
			"		step must max than 10 and less than 1M\n"
			"	-S num_start\n"
			"		if bulk read failed, set num_start by log\n"
			"	-T times_per_second\n"
			"		limit max keys read one seconds.This value must between 1-1000,\n"
			"		Default value is 1000/s, 0 means unlimited\n"
			"	-F keydump_file\n"
			"		get keys from keydump file\n",s);
	exit(0);
}

/* parse user input arguments */
static int parse_argv(int argc, char **argv)
{
	int iOpt = 0;
	int times_tmp = 0;
	if(argc < 5) {
		ShowUsage(argv[0]);
		return -1;
	}

#define V_OPT(argc, argv)	do {\
	if(!(optarg && optarg[0])) {\
		ShowUsage(argv[0]);\
		return -1;\
	}\
}while(0)

while((iOpt = getopt(argc, argv, "a:t:b:e:s:S:T:F:")) != -1)
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
			g_tail_begin = atoi(optarg);
			break;

		case 'e':
			V_OPT(argc, argv);
			g_tail_end = atoi(optarg);
			break;

		case 's':
			V_OPT(argc, argv);
			g_step = atoi(optarg);
			break;
		case 'S':
			V_OPT(argc, argv);
			g_setstart = atoi(optarg);
			break;
		case 'T':
			V_OPT(argc, argv);
			times_tmp = atoi(optarg);
			break;

		case 'F':
			V_OPT(argc, argv);
			g_dumpfile = optarg;
			break;
		default:
			fprintf(stderr, "invalid option: %c\n", iOpt);
			ShowUsage(argv[0]);
	}		
}

if(optind < argc)
{
	ShowUsage(argv[0]);
}

//校验输入参数的合法性

if (g_tail_begin < 0 || g_tail_begin > 999)
{
	fprintf(stderr,"tail_begin must between 0-999\n");
	ShowUsage(argv[0]);
}

if (g_tail_end < 0 || g_tail_end > 999)
{
	fprintf(stderr,"tail_end must between 0-999\n");
	ShowUsage(argv[0]);
}
else if (g_tail_end<g_tail_begin)
{
	fprintf(stderr,"tail_end must >= tail_begin\n");
	ShowUsage(argv[0]);
}

if (g_step < 10 || g_step > 1000000)
{
	fprintf(stderr,"step is illegal\n");
	ShowUsage(argv[0]);
}

//用于错误恢复
if (g_setstart)
{
	if ((g_setstart/1000) >g_start && (g_setstart/1000)<g_stop)
	{
		g_start = g_setstart;
		printf("continue from %u...\n", g_start*TAILMASK);
	}
	else
	{
		ShowUsage(argv[0]);
	}
}
//for freq limited	
if (times_tmp == 0)
{
	//unlimit the speed
	g_time_mutex = 0;
	printf("Max read freq is unlimit...\n");
}	
else if ((times_tmp) >0 && times_tmp<1000)
{
	g_time_mutex = 1000/times_tmp;
	printf("Max read freq is %d per second...\n", times_tmp);
}

//get keys from keydump file
if (g_dumpfile)
{
	printf("Get keys from dump file...\n");
	g_getkey_from_file = true;
}

return 0;
}

int main(int argc, char **argv)
{
	int iret = 0;
	if(parse_argv(argc, argv)) 
	{
		return -1;
	}

	//构建并配置server对象
	TTC::Server server;
	server.SetAddress(g_ttc_adress);
	server.SetTableName(g_tablename);
	server.SetMTimeout(1000);
	server.IntKey();



	if (g_getkey_from_file)
	{
		ifstream fin;
		fin.open(g_dumpfile);

		if (!fin)
		{
			printf("open keydump file:%s failed\n",g_dumpfile);
			return -1;
		}

		printf("start preread.....get keys from keydump file\n");

		iret = pre_read_use_dumpfile(fin, &server);
		if(iret)
		{
			fin.close();
			printf("pre read error\n");
			return iret;
		}
		else
		{
			fin.close();
			printf("pre read done!\n");
		}
		return iret;

	}
	else
	{
		//pre read from QQ_STAR toQQ_STOP
		g_start = QQ_START/TAILMASK;
		g_stop = QQ_STOP/TAILMASK;
		printf("start pre read.....\n");

		for (unsigned int i=g_tail_begin; i<=g_tail_end; i++)
		{
			iret = HandleBulkRead(i, &server);
			if(iret)
			{
				printf("tailnum is %3d:handle error\n", i);
				return iret;
			}
			else
			{
				printf(">tailnum is %3d:handle ok\n\n", i);
			}
		}
		return iret;

	}
}
