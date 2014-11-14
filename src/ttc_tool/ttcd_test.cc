#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

#include "ttcapi.h"
#include "config.h"
#include "timestamp.h"
#include "global.h"
#include "protocol.h"

#define STRCPY(d,s) do{ strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; }while(0)

#define GET_STR_CFG(s, k, d) do{ p = stCfgFile.GetStrVal(s, k); \
    if(p==NULL || p[0]==0){ \
	fprintf(stderr, "%s is null!\n", k); \
	return(-1); \
    } \
    STRCPY(d, p); \
}while(0)


typedef struct{
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

int GetConfig(const char* szCfgFileName)
{
    unsigned int i;
    const char* p;
    char szBuf[256];

    CConfig stCfgFile;

    if(stCfgFile.ParseConfig(szCfgFileName) < 0){
	fprintf(stderr, "get config error\n");
	return(-1);
    }

    GET_STR_CFG("TABLE_DEFINE", "TableName", g_szTableName);
    g_uiFieldCnt = stCfgFile.GetIntVal("TABLE_DEFINE", "FieldCount", 0);
    g_iKeyFieldCnt = stCfgFile.GetIntVal("TABLE_DEFINE", "KeyFieldCount", 1);
    if(g_uiFieldCnt < (unsigned int)g_iKeyFieldCnt){
	fprintf(stderr, "field count[%u] error, key field count[%u]\n", g_uiFieldCnt, g_iKeyFieldCnt);
	return(-2);
    }

    g_pstField = (CTTCField*)calloc(g_uiFieldCnt, sizeof(CTTCField));
    for(i=0; i<g_uiFieldCnt; i++){
	snprintf(szBuf, sizeof(szBuf), "FIELD%u", i+1);
	GET_STR_CFG(szBuf, "FieldName", g_pstField[i].m_szName);
	g_pstField[i].m_iType = stCfgFile.GetIntVal(szBuf, "FieldType", 0);
	g_pstField[i].m_uiSize = stCfgFile.GetIntVal(szBuf, "FieldSize", 0);
	p = stCfgFile.GetStrVal(szBuf, "Value");
	if(p== NULL || p[0]==0){
	    p = stCfgFile.GetStrVal(szBuf, "DefaultValue");
	    if(p!=NULL && p[0]!=0){
		g_pstField[i].m_iHasDefaultVal = 1;
		if(strcasecmp(p, "auto_increment") == 0)
		    g_iAutoIncField = i;
	    }
	    g_pstField[i].m_szVal[0] = 0;
	}
	else{
	    STRCPY(g_pstField[i].m_szVal, p);
	}

	if(g_pstField[i].m_iType < 1 || g_pstField[i].m_iType > 5){
	    fprintf(stderr, "field %d type error: %d\n", i, g_pstField[i].m_iType);
	    return(-3);
	}
    }

    return(0);
}

int SetKey(TTC::Request& stRequest, int iOp)
{
    int i;
    char* p=NULL;
    char* q=NULL;

    if(g_iKeyFieldCnt>1 && g_pstField[0].m_iType!=1 && g_pstField[0].m_iType!=2){
	fprintf(stderr, "multi-keys must be int or unsigned int.\n");
	return(-1);
    }

    switch(g_pstField[0].m_iType){
    case 1:
	if(g_szKeyValue[0] != 0) // 自增量key可以不指定值
	    stRequest.SetKey((strtoll(g_szKeyValue, &p, 10)));
	break;
    case 2:
	if(g_szKeyValue[0] != 0)
	    stRequest.SetKey((long long)strtoull(g_szKeyValue, &p, 10));
	break;
    case 4:
    case 5:
	stRequest.SetKey(g_szKeyValue);
	break;
    }
    for(i=1; i<g_iKeyFieldCnt; i++){
	if(g_pstField[i].m_iType!=1 && g_pstField[i].m_iType!=2){
	    fprintf(stderr, "multi-keys must be int or unsigned int.\n");
	    return(-1);
	}
	while(*p && !isdigit(*p))
	    p++;
	if(iOp != TTC::RequestInsert){
	    if(g_pstField[i].m_iType == 1)
		stRequest.EQ(g_pstField[i].m_szName, strtoll(p, &q, 10));
	    else
		stRequest.EQ(g_pstField[i].m_szName, (long long)strtoull(p, &q, 10));
	}
	else{
	    if(g_pstField[i].m_iType == 1)
		stRequest.Set(g_pstField[i].m_szName, strtoll(p, &q, 10));
	    else
		stRequest.Set(g_pstField[i].m_szName, (long long)strtoull(p, &q, 10));
	}
	p = q;
    }

    return(0);
}

int Get(TTC::Server& stServer)
{
    int iRet;
    unsigned int i, j;	
    int aiLimit[2];

    TTC::GetRequest stRequest(&stServer);
    if((iRet = SetKey(stRequest, TTC::RequestGet)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    for(i=g_iKeyFieldCnt; i<g_uiFieldCnt; i++){
	iRet = stRequest.Need(g_pstField[i].m_szName); // 设置需要select的字段
	if(iRet != 0){ 
	    fprintf(stderr, "set need %s error: %d\n", g_pstField[i].m_szName, iRet);
	    return(-2);
	}
    }
    if(g_szLimit[0] != 0){
	if(sscanf(g_szLimit, "%d%*[, ]%d", aiLimit, aiLimit+1) == 2){
	    stRequest.Limit(aiLimit[0], aiLimit[1]);
	}
	else if(sscanf(g_szLimit, "%d", aiLimit+1) == 1 && aiLimit[1]!=0){
	    stRequest.Limit(0, aiLimit[1]);
	}
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);

    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
	fprintf(stderr, "ttc execute get[%s] error: %d, %s, %s\n", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	return(-3); 
    } 

    if(stResult.NumRows() <= 0){ // 数据不存在
	printf("key[%s] data not exist.\n", g_szKeyValue);
	return(0); 
    } 

    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("key[%s] total-rows[%u], row-num[%d]:\n", g_szKeyValue, stResult.TotalRows(), stResult.NumRows());
    for(i=0; i<(unsigned int)stResult.NumRows(); i++){
	iRet = stResult.FetchRow();
	if(iRet < 0){ 
	    fprintf(stderr, "key[%s] ttc fetch row error: %d\n", g_szKeyValue, iRet);
	    return(-3); 
	}

	printf("--------------------------\n");
	for(j=1; j<g_uiFieldCnt; j++){
	    switch(g_pstField[j].m_iType){
	    case 1:
		printf("%s: %lld\n", g_pstField[j].m_szName, stResult.IntValue(g_pstField[j].m_szName));
		break;
	    case 2:
		printf("%s: %llu\n", g_pstField[j].m_szName, stResult.IntValue(g_pstField[j].m_szName));
		break;
	    case 3:
		printf("%s: %f\n", g_pstField[j].m_szName, stResult.FloatValue(g_pstField[j].m_szName));
		break;
	    case 4:
		printf("%s: %s\n", g_pstField[j].m_szName, stResult.StringValue(g_pstField[j].m_szName));
		break;
	    case 5:
        {
            int len=0;
            const char * buf = stResult.BinaryValue(g_pstField[j].m_szName,&len);
            printf("%s[%d]:", g_pstField[j].m_szName,len);
            for (int i=0;i<len;++i)
                i%2?printf("%x ",buf[i]):printf("%x",buf[i]);
            printf("\n");
        }
		break;
	    }
	}
    }

    return(1);
}

int Set(TTC::Server& stServer, int iOp)
{
    int iRet;
    int iUpdateField;
    unsigned int i;	

    TTC::Request stRequest(&stServer, iOp);
    if((iRet=SetKey(stRequest, iOp)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    iUpdateField = 0;
    for(i=g_iKeyFieldCnt; i<g_uiFieldCnt; i++){
	if((iOp==TTC::RequestInsert || iOp==TTC::RequestReplace) && !g_pstField[i].m_iHasDefaultVal && g_pstField[i].m_szVal[0]==0){
	    fprintf(stderr, "insert field[%s] require a value.\n", g_pstField[i].m_szName);
	    return(-1);
	}
	if(g_pstField[i].m_szVal[0] == 0)
	    continue;

	iUpdateField++;
	switch(g_pstField[i].m_iType){
	case 1:
	    iRet = stRequest.Set(g_pstField[i].m_szName, strtoll(g_pstField[i].m_szVal, NULL, 10));
	    break;
	case 2:
	    iRet = stRequest.Set(g_pstField[i].m_szName, (long long)strtoull(g_pstField[i].m_szVal, NULL, 10));
	    break;
	case 3:
	    iRet = stRequest.Set(g_pstField[i].m_szName, strtod(g_pstField[i].m_szVal, NULL));
	    break;
	case 4:
	    iRet = stRequest.Set(g_pstField[i].m_szName, g_pstField[i].m_szVal);
	    break;
	case 5:
	    iRet = stRequest.Set(g_pstField[i].m_szName, g_pstField[i].m_szVal, strlen(g_pstField[i].m_szVal));
	    break;
	}
	if(iRet != 0){ 
	    fprintf(stderr, "set field[%s] error: %d\n", g_pstField[i].m_szName, iRet);
	    return(-2);
	}
    }
    if((iOp!=TTC::RequestInsert && iOp!=TTC::RequestReplace) && iUpdateField < 1){
	fprintf(stderr, "update field not found!\n");
	return(-3);
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
	fprintf(stderr, "ttc execute %s[%s] error: %d, %s, %s\n", iOp==TTC::RequestInsert?"insert":"update", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	return(-3); 
    }
    if(iOp==TTC::RequestInsert && g_szKeyValue[0]==0 && g_pstField[0].m_iHasDefaultVal==1)
	snprintf(g_szKeyValue, sizeof(g_szKeyValue), "%lld", stResult.InsertID());

    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("%s key[%s] success.", iOp==TTC::RequestInsert?"insert":iOp==TTC::RequestReplace?"replace":"update", g_szKeyValue);
    if((iOp==TTC::RequestInsert || iOp==TTC::RequestReplace) && g_iAutoIncField > 0)
	printf("insert id: %lld\n", stResult.InsertID());
//    else if(iOp==TTC::RequestUpdate)
	printf("affected rows: %d\n", stResult.AffectedRows());
//    else
//	printf("\n");

    return(0);
}

int DeleteAll(TTC::Server& stServer)
{
    int iRet;

    TTC::Request stRequest(&stServer, TTC::RequestDelete);
    if((iRet=SetKey(stRequest, TTC::RequestDelete)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();

    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);

    llEnd = GET_TIMESTAMP();

    if(iRet != 0){ 
	    fprintf(stderr, "ttc execute delete[%s] error: %d, %s, %s\n", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	    return(-3); 
    } 

    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("delete key[%s] success, affected-rows: %d\n", g_szKeyValue, stResult.AffectedRows());

    return(0);
}

int Delete(TTC::Server& stServer)
{
    int iRet;
    unsigned int i;

    TTC::Request stRequest(&stServer, TTC::RequestDelete);
    if((iRet=SetKey(stRequest, TTC::RequestDelete)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    for(i=g_iKeyFieldCnt; i<g_uiFieldCnt; i++){
	if(g_pstField[i].m_szVal[0] == 0)
	    continue;

	switch(g_pstField[i].m_iType){
	case 1:
	    iRet = stRequest.EQ(g_pstField[i].m_szName, strtoll(g_pstField[i].m_szVal, NULL, 10));
	    break;
	case 2:
	    iRet = stRequest.EQ(g_pstField[i].m_szName, (long long)strtoull(g_pstField[i].m_szVal, NULL, 10));
	    break;
	case 3:
	    fprintf(stderr, "not support float condition yet: field[%s]", g_pstField[i].m_szName);
	    return(-2);
	case 4:
	    iRet = stRequest.EQ(g_pstField[i].m_szName, g_pstField[i].m_szVal);
	    break;
	case 5:
	    iRet = stRequest.EQ(g_pstField[i].m_szName, g_pstField[i].m_szVal, strlen(g_pstField[i].m_szVal));
	    break;
	}
	if(iRet != 0){ 
	    fprintf(stderr, "set EQ %s error: %d\n", g_pstField[i].m_szName, iRet);
	    return(-2);
	}
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
	fprintf(stderr, "ttc execute delete[%s] error: %d, %s, %s\n", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	return(-3); 
    } 
    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("delete key[%s] success, affected-rows: %d\n", g_szKeyValue, stResult.AffectedRows());

    return(0);
}

int Purge(TTC::Server& stServer)
{
    int iRet;

    TTC::Request stRequest(&stServer, TTC::RequestPurge);
    if((iRet=SetKey(stRequest, TTC::RequestPurge)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
	fprintf(stderr, "ttc execute purge[%s] error: %d, %s, %s\n", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	return(-3); 
    } 
    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("purge key[%s] success\n", g_szKeyValue);

    return(0);
}

int Flush(TTC::Server& stServer)
{
    int iRet;

    TTC::Request stRequest(&stServer, TTC::RequestFlush);
    if((iRet=SetKey(stRequest, TTC::RequestFlush)) != 0){
	fprintf(stderr, "set key error: %d\n", iRet);
	return(-1);
    }

    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
	fprintf(stderr, "ttc execute flush[%s] error: %d, %s, %s\n", g_szKeyValue, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
	return(-3); 
    } 
    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("flush key[%s] success\n", g_szKeyValue);

    return(0);
}

int SetReadOnly(TTC::Server& stServer, int iReadOnly)
{
   int iRet;

    TTC::Request stRequest(&stServer, TTC::RequestSvrAdmin);
	if(iReadOnly)
		stRequest.SetAdminCode(TTC::SET_READONLY);
	else
		stRequest.SetAdminCode(TTC::SET_READWRITE);
	
    int64_t llBegin, llEnd;
    llBegin = GET_TIMESTAMP();
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    llEnd = GET_TIMESTAMP();
    if(iRet != 0){ 
		fprintf(stderr, "ttc execute set %s error: %d, %s, %s\n", iReadOnly?"readonly":"read-write", iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
		return(-3); 
    } 
    printf("query use "UINT64FMT" microseconds.\n", llEnd - llBegin);
    printf("set %s success\n", iReadOnly?"readonly":"read-write");

    return(0);	
}

void ShowUsage(const char* szName)
{
    printf("usage: %s -t table_conf_file -k key_value -o operation -i ttc_ip -p ttc_port [-l m,n]\n", szName);
    printf("	-t table_conf_file\n");
    printf("		ttc's table.conf path\n");
    printf("	-k key_value\n");
    printf("		what key to test\n");
    printf("	-o operation\n");
    printf("		get|insert|replace|update|delete|deleteall|purge|flush|readonly|readwrite\n");
    printf("	-i ttc_ip\n");
    printf("		ttc server ip or unix-socket path\n");
    printf("	-p ttc_port\n");
    printf("		ttc server port\n");
    printf("	-l m,n\n");
    printf("		limit m,n for get operation\n");
    printf("	-a access_token\n");
    printf("		ttc's accss_token\n");
    printf("	-w log_level\n");
    printf("        ttc's log_level,default 0-emerg, 1-alert, 2-crit, 3-error, "
    				"4-warning, 5-notice, 6-info,	7-debug\n");

    return;
}

inline unsigned int SafeStrtoull(const char * sNumber)
{
	unsigned int i = 0;
    if(sNumber == NULL)
    {
        return 0;
    }
    i = strtoull(sNumber, NULL, 10);
    if(i > 7)
    {
    	i = 0;
    }
    return i;
}

int main(int argc, char* argv[])
{
    int iRet;
    int iOpt;
    char szCfgFile[256];
    char szOperation[12];
    char szIP[64];
    char szPort[12];
    char szAccessToken[128];
    char* p;
    unsigned int lv = 0;


    memset(szCfgFile, 0, sizeof(szCfgFile));
    memset(g_szKeyValue, 0, sizeof(g_szKeyValue));
    memset(szOperation, 0, sizeof(szOperation));
    memset(szIP, 0, sizeof(szIP));
    memset(szPort, 0, sizeof(szPort));
    memset(g_szLimit, 0, sizeof(g_szLimit));
    memset(szAccessToken, 0, sizeof(szAccessToken));
    while((iOpt = getopt(argc, argv, "t:k:o:i:p:l:a:w:")) != -1){
	switch(iOpt){
	case 't':
	    STRCPY(szCfgFile, optarg);
	    break;

	case 'k':
	    STRCPY(g_szKeyValue, optarg);
	    break;

	case 'o':
	    STRCPY(szOperation, optarg);
	    break;

	case 'i':
	    STRCPY(szIP, optarg);
	    break;

	case 'p':
	    STRCPY(szPort, optarg);
	    break;

	case 'l':
	    STRCPY(g_szLimit, optarg);
	    break;

	case 'a':
		STRCPY(szAccessToken, optarg);
		break;

	case 'w':
		lv = SafeStrtoull(optarg);
		break;

	default:
	    fprintf(stderr, "invalid option: %c\n", iOpt);
	    ShowUsage(argv[0]);
	    exit(1);
	}
    }
    if(szCfgFile[0]==0 || szOperation[0]==0 || szIP[0]==0){
	ShowUsage(argv[0]);
	exit(1);
    }
    if(szIP[0]=='@' || szIP[0]=='/'){ // unix-socket
	szPort[0] = 0;
    }
    else if((p=strchr(szIP, ':'))!= NULL){
	snprintf(szPort, sizeof(szPort), "%s", p+1);
	*p = 0;
    }
    else{
	if(szPort[0] == 0){
	    ShowUsage(argv[0]);
	    exit(1);
	}
    }

    iRet = GetConfig(szCfgFile);
    if(iRet != 0)
	exit(2);

    if(g_szKeyValue[0]==0 && strcasecmp(szOperation, "readonly")!=0 && strcasecmp(szOperation, "readwrite")!=0 && ((strcasecmp(szOperation, "insert")!=0 && strcasecmp(szOperation, "replace")!=0) || g_pstField[0].m_iHasDefaultVal==0) ){
		// 除了insert/replace操作，并且是自增量key以外，必须指定key值。
		ShowUsage(argv[0]);
		exit(1);
    }
    TTC::init_log("dtc_client");
    TTC::set_log_level(lv);
    TTC::Server stServer; // 只要server不析构，后台会保持长连接
    switch(g_pstField[0].m_iType){
    case 1:
    case 2:
	stServer.IntKey();
	break;

    case 4:
	stServer.StringKey();
	break;

    case 5:
	stServer.BinaryKey();
	break;

    default:
	fprintf(stderr, "invalide key type[%d]\n", g_pstField[0].m_iType);
	exit(3);
    }
    stServer.SetTableName(g_szTableName);
    stServer.SetAddress(szIP, szPort);
    stServer.SetTimeout(5); 
    stServer.SetAccessKey(szAccessToken);

    if(strcasecmp(szOperation, "get") == 0){
		iRet = Get(stServer);
    }
    else if(strcasecmp(szOperation, "insert") == 0){
	iRet = Set(stServer, TTC::RequestInsert);
    }
	else if(strcasecmp(szOperation, "replace") == 0){
	iRet = Set(stServer, TTC::RequestReplace);
    }
    else if(strcasecmp(szOperation, "update") == 0){
	iRet = Set(stServer, TTC::RequestUpdate);
    }
    else if(strcasecmp(szOperation, "delete") == 0){
	iRet = Delete(stServer);
    }
    else if(strcasecmp(szOperation, "deleteall") == 0){
	iRet = DeleteAll(stServer);
    }
    else if(strcasecmp(szOperation, "purge") == 0){
	iRet = Purge(stServer);
    }
	else if(strcasecmp(szOperation, "flush") == 0){
	iRet = Flush(stServer);
    }
    else if(strcasecmp(szOperation, "readonly") == 0){
	iRet = SetReadOnly(stServer, 1);
    }
	else if(strcasecmp(szOperation, "readwrite") == 0){
		iRet = SetReadOnly(stServer, 0);
	}
    else{
	fprintf(stderr, "invalid operation: %s\n", szOperation);
	exit(4);
    }

    free(g_pstField);

    return(0);
}

