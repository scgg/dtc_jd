/*
 * =====================================================================================
 *
 *       Filename:  mem_defragment.cc
 *
 *    Description:  defragment mem of ttc
 *
 *        Version:  1.0
 *        Created:  07/17/2010 01:25:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "dbconfig.h"
#include "log.h"
#include "defragment.h"
#define DATADIR "../data"

#define STRCPY(d,s) do{ strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; }while(0)

//因为只是工具，所以程序中有读取配置、Server对象new后没有delete等可以预知的内存泄漏
//进程退出后会随进程被删除

static char gCacheCfg[512];//ttc's cache.conf file path
static char gTabCfg[512];// ttc's table.conf file path
static int gLevel; //-l level
static int gStep; // -s step
static bool gVerbose; //-v or not
static char * gHandleFile; // -h handle file path
int gMaxErrorCount; // -c count

CConfig *gConfig;
CDbConfig* dbConfig;
CTableDefinition *gTableDef;


/*
 * read table.conf and cache.conf
 * and pause config file
 * */
int InitConf(const char* szCacheCfg, const char* szTabCfg)
{
	gConfig = new CConfig;
	if(gConfig->ParseConfig(szCacheCfg, "cache")){
		fprintf(stderr,"parse cache config error.\n");
		return -1;
	}

	dbConfig = CDbConfig::Load(szTabCfg);
	if(dbConfig == NULL){
		fprintf(stderr,"load table configuire file error.\n");
		return -2;
	}

	gTableDef = dbConfig->BuildTableDefinition();
	if(gTableDef == NULL){
		fprintf(stderr,"build table definition error.\n");
		return -3;
	}

	return 0;
}

/*
 * init server for proccess
 * */
TTC::Server * InitServer()
{
	int iRet = 0;
	TTC::Server *s = new TTC::Server;
	if (s == NULL)
	{
		log_error("new server error");
		return NULL;
	}
	s->IntKey();//只是ping一下，可以随便设置
	s->SetTableName("*");
	s->SetAddress(gConfig->GetStrVal("cache","BindAddr"));
	s->SetTimeout(3000);
	iRet = s->Ping();
	if(iRet && iRet != -TTC::EC_TABLE_MISMATCH)
	{
		log_error("ping ttc[%s] error: %d\n",gConfig->GetStrVal("cache","BindAddr"),iRet);
		delete s;
		return NULL;
	}

	return s;
}

void ShowUsage(const char* szName)
{
	printf("usage: %s -f cache_conf_file -t table_conf_file -l level \n", szName);
	printf("	-f cache_conf_file\n");
	printf("		ttc's cache.conf path\n");
	printf("	-t table_conf_file\n");
	printf("		ttc's table.conf path\n");
	printf("	-v\n");
	printf("		dump all chunks to stderr\n");
	printf("	-l level\n");
	printf("		mem defragment level.level 1 - level 100.\n");
	printf("	-s step\n");
	printf("		step per second.1-1000000.0 is no limit\n");
	printf("\t\thigh level will consume more time and cpu.\n");

	return;
}


int main(int argc,char **argv)
{
	int iRet = 0;
	int iOpt = 0;

	memset(gTabCfg, 0, sizeof(gTabCfg));
	memset(gCacheCfg, 0, sizeof(gCacheCfg));
	gLevel = 1;
	gStep = 0;
	gVerbose = false;
	bool OldMode = false;

	while((iOpt = getopt(argc, argv, "f:t:l:vh:s:c:o")) != -1){
		switch(iOpt){
			case 'f':
				STRCPY(gCacheCfg, optarg);
				break;

			case 't':
				STRCPY(gTabCfg, optarg);
				break;

			case 'l':
				gLevel = atoi(optarg);
				break;

			case 'v':
				gVerbose = true;
				break;
			case 'o':
				OldMode = true;
				break;

			case 'h':
				gHandleFile = strdup(optarg);
				break;
			case 'c':
				gMaxErrorCount = atoi(optarg);
				break;

			case 's':
				gStep = atoi(optarg);
				break;

			default:
				fprintf(stderr, "invalid option: %c\n", iOpt);
				ShowUsage(argv[0]);
				exit(1);
		}
	}

	if(gTabCfg[0] == 0 || gCacheCfg[0] == 0){
		ShowUsage(argv[0]);
		exit(1);
	}

	/*
	 * level must between 1-100
	 * */
	gLevel = gLevel<1?1:(gLevel>100?100:gLevel);

	/*step must berween 1-1000000 or 0 no limit*/
	gStep = gStep<0?0:(gStep>1000000?1000000:gStep);


	gMaxErrorCount = gMaxErrorCount<=0?4:gMaxErrorCount;
	iRet = InitConf(gCacheCfg, gTabCfg);
	_init_log_("mem_defragment");
	_set_log_level_(gConfig->GetIdxVal("cache", "LogLevel",
				((const char * const []){
				 "emerg",
				 "alert",
				 "crit",
				 "error",
				 "warning",
				 "notice",
				 "info",
				 "debug",
				 NULL }), 6));
	_set_log_thread_name_("mem_defragment");

	if(iRet != 0)
		exit(2);

	mkdir(DATADIR, 0777);

	if(access(DATADIR, W_OK|X_OK) < 0)
	{
		log_error("dir(%s) Not writable", DATADIR);
		return -1;
	}

	CDefragment holder;
	if (holder.Attach(gConfig->GetStrVal("cache", "CacheShmKey"),gTableDef->KeySize(),gStep))
	{
		return -1;
	}
	//just dump mem detail to stderr
	if (gVerbose)
	{
		if (holder.DumpMem(gVerbose))
		{
			log_error("dump bins error");
			return -1;
		}
		return 0;
	}

	TTC::Server * s = InitServer();

	//if -h process handle file's handle
	if (gHandleFile)
	{
		if (holder.ProccessHandle(gHandleFile,s))
		{
			log_error("ProccessHandle error");
			return -1;
		}
	}

	if (OldMode)
	{//老的模式的边扫边整理，有时候会遇到chunk被改动的情况
		//dump chunk stat before Defragment
		if (holder.DumpMem())
		{
			log_error("dump chunk stat before Defragment error");
			return -1;
		}

		//Do MEM DEFRAGM
		if (holder.DefragMem(gLevel,s))
		{
			log_error("Defragment error");
			return -1;
		}
	}
	else//新的内存整理模式是扫完放到文件再来处理
	{
		char filename[1024];
		snprintf(filename,sizeof(filename),"%s/mem_defragement_%d",DATADIR,(int)getpid());
		log_notice("write dump file:%s",filename);
		uint64_t memsize = 1;
		if (holder.DumpMemNew(filename,memsize))
		{
			log_error("dump chunk stat before Defragment error");
			log_notice("delete dump file:%s ret:%d",filename,unlink(filename));
			return -1;
		}

		if (holder.DefragMemNew(gLevel,s,filename,memsize))
		{
			log_error("Defragment may error");
		}

		log_notice("delete dump file:%s ret:%d",filename,unlink(filename));
	}

	//dump chunk stat after Defragment
	if (holder.DumpMem())
	{
		log_error("dump chunk stat after Defragment error");
		return -1;
	}

	DELETE(gConfig);
	DELETE(dbConfig);
	DELETE(gTableDef);

}
