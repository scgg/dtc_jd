#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ttcapi.h"
#include "config.h"
#include "timestamp.h"
#include "global.h"
#include "protocol.h"
#include "value.h"
#include "mutexwait.h"

#define STRCPY(d,s) do{ strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; }while(0)
#define KEY_PER_PROCESS 5000

#define GET_STR_CFG(s, k, d) do{ p = stCfgFile.GetStrVal(s, k); \
	if(p==NULL || p[0]==0){ \
		fprintf(stderr, "%s is null!\n", k); \
		return(-1); \
	} \
	STRCPY(d, p); \
}while(0)


typedef struct
{
	char m_szName[256];
	int	m_iType;
	unsigned int m_uiSize;
	int m_iHasDefaultVal;
	char m_szVal[256];
}CTTCField;

int g_iKeyFieldCnt;
int g_iAutoIncField=-1;
unsigned int g_uiFieldCnt;
CTTCField* g_pstField;
char g_szTableName[256];
char g_szKeyValue[1024];
char g_szLimit[40];
long long *keylist = NULL;
unsigned int multi_num = 0;
unsigned int linenum = 0;

int GetConfig(const char* szCfgFileName)
{
	unsigned int i;
	const char* p;
	char szBuf[256];

	CConfig stCfgFile;

	if(stCfgFile.ParseConfig(szCfgFileName) < 0)
	{
		fprintf(stderr, "get config error\n");
		return(-1);
	}

	GET_STR_CFG("TABLE_DEFINE", "TableName", g_szTableName);
	g_uiFieldCnt = stCfgFile.GetIntVal("TABLE_DEFINE", "FieldCount", 0);
	g_iKeyFieldCnt = stCfgFile.GetIntVal("TABLE_DEFINE", "KeyFieldCount", 1);
	if(g_uiFieldCnt < (unsigned int)g_iKeyFieldCnt)
	{
		fprintf(stderr, "field count[%u] error, key field count[%u]\n", g_uiFieldCnt, g_iKeyFieldCnt);
		return(-2);
	}

	g_pstField = (CTTCField*)calloc(g_uiFieldCnt, sizeof(CTTCField));
	for(i=0; i<g_uiFieldCnt; i++)
	{
		snprintf(szBuf, sizeof(szBuf), "FIELD%u", i+1);
		GET_STR_CFG(szBuf, "FieldName", g_pstField[i].m_szName);
		g_pstField[i].m_iType = stCfgFile.GetIntVal(szBuf, "FieldType", 0);
		g_pstField[i].m_uiSize = stCfgFile.GetIntVal(szBuf, "FieldSize", 0);
		p = stCfgFile.GetStrVal(szBuf, "Value");
		if(p== NULL || p[0]==0)
		{
			p = stCfgFile.GetStrVal(szBuf, "DefaultValue");
			if(p!=NULL && p[0]!=0)
			{
				g_pstField[i].m_iHasDefaultVal = 1;
				if(strcasecmp(p, "auto_increment") == 0)
					g_iAutoIncField = i;
			}
			g_pstField[i].m_szVal[0] = 0;
		}
		else
		{
			STRCPY(g_pstField[i].m_szVal, p);
		}

		if(g_pstField[i].m_iType < 1 || g_pstField[i].m_iType > 5)
		{
			fprintf(stderr, "field %d type error: %d\n", i, g_pstField[i].m_iType);
			return(-3);
		}
	}
	return 0;
}

int DoExist(TTC::Server &sstServer,FILE *&fp,unsigned long begin,unsigned long end,long long *existkey,unsigned int numrow)
{
	int iRet = 0;
	for(unsigned long id = begin; id <= end; id++)
	{
		unsigned int i = 0;
		for(i = 0; i< numrow;i++)
		{
			if(keylist[id] == existkey[i])
				break;
		}
		if(i < numrow)
		{
			continue;
		}
		TTC::GetRequest sstRequest(&sstServer);
		iRet = sstRequest.AddKeyValue(g_pstField[0].m_szName,keylist[id]);
		if(iRet != 0)
		{
			fprintf(stderr, "set key error: %d\n", iRet);
			return(-1);
		}

		for(i=0; i<g_uiFieldCnt; i++)
		{   
			iRet = sstRequest.Need(g_pstField[i].m_szName); // 设置需要select的字段
			if(iRet != 0)
			{   
				fprintf(stderr, "set need %s error: %d\n", g_pstField[i].m_szName, iRet);
				return(-2);
			}   
		}
		
		TTC::Result sstResult;
		iRet = sstRequest.Execute(sstResult);
		if(sstResult.NumRows() != 0)
		{
			fprintf(fp,"%lld\tmaster\n",keylist[id]);
			//return(-1);
		}
	}
	
	return 0;
}

int Diff(TTC::Result &sstResult,TTC::Result &bstResult,FILE *&fp)
{
	for(unsigned int j=1; j<g_uiFieldCnt; j++)
	{
		switch(g_pstField[j].m_iType)
		{
			case 1:
			case 2:
				{
					if(sstResult.IntValue(g_pstField[j].m_szName) != bstResult.IntValue(g_pstField[j].m_szName))
					{
//						fprintf(fp,"\n%d\t%lld\t%lld\n",j,sstResult.IntValue(g_pstField[j].m_szName),bstResult.IntValue(g_pstField[j].  m_szName));
						return -1;
					}
					break;
				}
			case 3:
				{
					double s_value = sstResult.FloatValue(g_pstField[j].m_szName);
					double t_value = bstResult.FloatValue(g_pstField[j].m_szName);
					double diff_value = s_value - t_value;
					if(diff_value > 0.00001 || diff_value < -0.00001)
					{
//						fprintf(fp,"\n%f\t%f\n",s_value,t_value);
						return -1;
					}
					break;
				}
			case 4:
				{
					const char *s_value = sstResult.StringValue(g_pstField[j].m_szName);
					const char *t_value = bstResult.StringValue(g_pstField[j].m_szName);

					if((s_value== NULL && t_value == NULL)||(s_value != NULL && t_value == NULL)||(s_value == NULL && t_value != NULL))
					{
//						fprintf(fp,"\n%s\t%s\t%s\n",g_pstField[j].m_szName,s_value,t_value);
						return -1;
					}

					if(strcmp(s_value,t_value) != 0)
					{
//						fprintf(fp,"\nnot null %s\t%s\t%s\n",g_pstField[j].m_szName,s_value,t_value);	
						return -1;
					}
					break;
				}
			case 5:
				{
					const char *s_value = sstResult.BinaryValue(g_pstField[j].m_szName);
					const char *t_value = bstResult.BinaryValue(g_pstField[j].m_szName);

					if((s_value== NULL && t_value == NULL)||(s_value != NULL && t_value == NULL)||(s_value == NULL && t_value != NULL))
					{
//						fprintf(fp,"\n%s\t%s\t%s\n",g_pstField[j].m_szName,s_value,t_value);
						return -1;
					}

					if(strcmp(s_value,t_value) != 0)
					{
//						fprintf(fp,"\nnot null %s\t%s\t%s\n",g_pstField[j].m_szName,s_value,t_value);	
						return -1;
					}
					break;
				}
		}
	}
	return 0;
}

int DoDiff(TTC::Server &sstServer,TTC::Result &bstResult,FILE *&fp,unsigned long begin,unsigned long end)
{	
	int iret = 0;
	long long *existkey = NULL;
	unsigned int tmpid = 0;
	unsigned int snumrow = bstResult.NumRows();
	existkey = (long long *)malloc(snumrow * sizeof(long long));
	
	for(unsigned int i=0; i<snumrow; i++)
	{
		iret = bstResult.FetchRow();
		
		long long key = bstResult.IntValue(g_pstField[0].m_szName);
		existkey[tmpid] = key;
		tmpid++;
		TTC::GetRequest sstRequest(&sstServer);
		iret = sstRequest.AddKeyValue(g_pstField[0].m_szName,key);
		if(iret != 0)
		{
			fprintf(stderr, "set key[t] error: %d\n", iret);
			return(-1);
		}

		for(unsigned int j=0; j<g_uiFieldCnt; j++)
		{   
			iret = sstRequest.Need(g_pstField[j].m_szName); // 设置需要select的字段
			if(iret != 0)
			{   
				fprintf(stderr, "set need %s error: %d\n", g_pstField[j].m_szName, iret);
				return(-2);
			}   
		}
		
		TTC::Result sstResult;
		iret = sstRequest.Execute(sstResult);
		if(iret != 0)
			return -1;
		
		if(sstResult.FetchRow())
		{
			fprintf(fp,"%lld\tslave\n",key);
			continue;
		}
		else if(Diff(sstResult,bstResult,fp) != 0)
		{
			fprintf(fp,"%lld\tdiff\n",key);
		}
	}
	
	DoExist(sstServer,fp,begin, end,existkey,snumrow);
	free(existkey);
	return 0;
}

int Get(TTC::Server &sstServer,TTC::Server &bstServer,FILE *&fp,unsigned long begin,unsigned long end)
{
	int iRet = 0;
	TTC::GetRequest bstRequest(&bstServer);

	for(unsigned long id = begin; id <= end; id++)
	{
		iRet += bstRequest.AddKeyValue(g_pstField[0].m_szName,keylist[id]);

	}
	if(iRet != 0)
	{   
		fprintf(stderr, "set key error: %d\n", iRet);
		return(-1);
	}   

	for(unsigned int i = 0; i<g_uiFieldCnt; i++)
	{   
		iRet = bstRequest.Need(g_pstField[i].m_szName); // 设置需要select的字段
		if(iRet != 0)
		{   
			fprintf(stderr, "set need %s error: %d\n", g_pstField[i].m_szName, iRet);
			return(-2);
		}   
	}   

	TTC::Result bstResult;
	bstRequest.Execute(bstResult);
	if( iRet != 0)
	{   
		return -1; 
	}  
	DoDiff(sstServer,bstResult,fp,begin,end);
	return 0;
//	return DoDiff(sstServer,bstResult,fp);
}

void ShowUsage(const char* szName)
{
	printf("usage: %s -t table_conf_file -i s_ttc_ip -p s_ttc_port -I b_ttc_ip -P b_ttc_port -n num -N prcess_num -e keynum_for_process -o errkeyfile -m mulitkey_num\n", szName);
	printf("	-t table_conf_file\n");
	printf("		ttc's table.conf path\n");
	printf("	-i s_ttc_ip\n");
	printf("		source ttc server ip or unix-socket path\n");
	printf("	-p s_ttc_port\n");
	printf("		source ttc server port\n");
	printf("	-I b_ttc_ip\n");
	printf("		back ttc server ip or unix-socket path\n");
	printf("	-P b_ttc_port\n");
	printf("		back ttc server port\n");
	printf("	-n num\n");
	printf("		the time in one second to diff\n");
	printf("   	-f filename\n");
	printf("		the file have the key\n");
	printf("   	-o filename\n");
	printf("		the file to write the error key\n");
	printf("	-N process_num\n");
	printf("		the num of process to deal the key\n");
	printf("	-e keynum_for_process\n");
	printf("		the num each process to deal\n");
	printf("	-m multikey_num\n");
	printf("		the num of key to get value one time\n");
	return;
}

int main(int argc, char* argv[])
{
	int iRet;
	int iOpt;
	char szCfgFile[256];
	char szsIP[64];
	char szsPort[12];
	char szbIP[64];
	char szbPort[12];
	char sznum[10];
	char szfile[50];
	char errkeyfile[50];
	char tmpstr[50];
	unsigned int num = 10000;
	unsigned int process_num = 0;
	unsigned int keynum_process = 0;
	memset(szCfgFile, 0, sizeof(szCfgFile));
	memset(szsIP, 0, sizeof(szsIP));
	memset(szsPort, 0, sizeof(szsPort));
	memset(szbIP, 0, sizeof(szbIP));
	memset(szbPort, 0, sizeof(szbPort));
	memset(sznum,0,sizeof(sznum));
	memset(szfile,0,sizeof(szfile));
	memset(errkeyfile,0x0,sizeof(errkeyfile));

	while((iOpt = getopt(argc, argv, "t:i:p:I:P:n:f:o:N:e:m:")) != -1)
	{
		switch(iOpt)
		{
			case 't':
				STRCPY(szCfgFile, optarg);
				break;

			case 'i':
				STRCPY(szsIP, optarg);
				break;

			case 'p':
				STRCPY(szsPort, optarg);
				break;

			case 'I':
				STRCPY(szbIP, optarg);
				break;

			case 'P':
				STRCPY(szbPort, optarg);
				break;
			case 'n':
				STRCPY(sznum, optarg);
				break;
			case 'f':
				STRCPY(szfile, optarg);
				break;
			case 'o':
				STRCPY(errkeyfile,optarg);
				break;
			case 'N':
				STRCPY(tmpstr,optarg);
				process_num = strtoul(tmpstr,NULL,10);
				break;
			case 'e':
				STRCPY(tmpstr,optarg);
				keynum_process = strtoul(tmpstr,NULL,10);
				break;
			case 'm':
				STRCPY(tmpstr,optarg);
				multi_num = strtoul(tmpstr,NULL,10);
				break;
			default:
				fprintf(stderr, "invalid option: %c\n", iOpt);
				ShowUsage(argv[0]);
				exit(1);
		}
	}

	if(szCfgFile[0]==0 || szsIP[0]==0 || szsPort[0]==0 || szbIP[0]==0 || szbPort[0]==0 ||errkeyfile[0] == 0)
	{
		ShowUsage(argv[0]);
		exit(1);
	}


	iRet = GetConfig(szCfgFile);

	if(iRet != 0)
		exit(2);

	num = strtoul(sznum,NULL,10);
	if(num >= 100000)
	{
		num = 100000;
	}


	FILE *keyfp = fopen(szfile,"r+");
	if(keyfp == NULL)
	{
		fprintf(stderr,"open file[%s] error.\n",szfile);
		return 3;
	}

	//默认不一致的key放在keylist.log里面
	char tmpinfo[1024];
	memset(tmpinfo,0x0,sizeof(tmpinfo));
	mutexwait mywait;

	while(fgets(tmpinfo,sizeof(tmpinfo),keyfp))
	{
		linenum++;
	}

	if(process_num == 0 && keynum_process == 0)
	{
		process_num = (linenum -1)/KEY_PER_PROCESS + 1;	
	}
	else if(process_num == 0 && keynum_process != 0)
	{
		process_num = linenum % keynum_process;

	}

	if(multi_num == 0)
	{
		multi_num = 1;
	}
	
	if(multi_num > 1000)
	{
		multi_num = 1000;
	}

	keylist = (long long *)malloc(linenum * sizeof(long long));
	int tmpid = 0;

	if(fseek(keyfp,0,SEEK_SET))
	{
		fprintf(stderr,"rewind keydump file faild");
		return 0;
	}

	while(fgets(tmpinfo,sizeof(tmpinfo),keyfp))
	{
		keylist[tmpid] = strtol(tmpinfo,NULL,10);
		tmpid++;
	}

	pid_t pid;
	for(unsigned int i = 0 ;i < process_num; i++)
	{
		pid=fork();  
		if(pid ==0)  
		{
		//	unsigned long long keynum = 0;
			unsigned long begin_id = 0;
			unsigned long end_id = 0;
			
			if(i < linenum % process_num)
			{
				begin_id = i * (linenum/process_num +1);
				end_id = begin_id + linenum/process_num ;
			}
			else
			{
				begin_id = (linenum % process_num)*(linenum/process_num + 1) +(i - linenum % process_num)*(linenum/process_num);
				end_id = begin_id + linenum/process_num - 1; 
			}

			TTC::Server sstServer; // 只要server不析构，后台会保持长连接
			sstServer.AddKey(g_pstField[0].m_szName, TTC::KeyTypeInt);
			sstServer.SetTableName(g_szTableName);
			sstServer.SetAddress(szsIP, szsPort);
			sstServer.SetTimeout(5); 

			TTC::Server bstServer; // 只要server不析构，后台会保持长连接
			bstServer.AddKey(g_pstField[0].m_szName, TTC::KeyTypeInt);
			bstServer.SetTableName(g_szTableName);
			bstServer.SetAddress(szbIP, szbPort);
			bstServer.SetTimeout(5);
			TTC::set_key_value_max(2000);	
			
			char difflog[100];
			snprintf(difflog,sizeof(difflog),"%s_%d.log",errkeyfile,i);

			FILE *logfp = fopen(difflog,"w+");
			if (logfp == NULL)
			{
				fprintf(stderr,"open file[%s] error.\n",szfile);
				return 0;
			}

			unsigned long tempbegin = begin_id;
			unsigned long tempend = 0;
			while(tempbegin < end_id)
			{
				tempend = ((tempbegin + multi_num - 1) >= end_id ) ? end_id :(tempbegin + multi_num - 1);

				Get(sstServer,bstServer,logfp,tempbegin,tempend);
				
				tempbegin = tempend + 1;
			}
			fclose(logfp);
			exit(0);

		}      
		else if (pid<0)
		{            
			printf("fork error\n");
		}     

	}
	int xx =0; 
	do   
	{     
		xx=waitpid(-1,NULL,0);  
	}    
	while(xx>0);

	fclose(keyfp);
	
	free(keylist);
	free(g_pstField);

	return(0);
}

