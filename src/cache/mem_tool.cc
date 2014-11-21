#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbconfig.h"
#include "DBReader.h"
#include "DBWriter.h"
#include "TxtReader.h"
#include "TxtWriter.h"
#include "cache_reader.h"
#include "cache_writer.h"
#include "value.h"
#include "field.h"
#include "log.h"

#include "cb_reader.h"
#include "cb_decoder.h"
#include "raw_data.h"

#define STRCPY(d,s) do{ strncpy(d, s, sizeof(d)-1); d[sizeof(d)-1]=0; }while(0)
#define DUMP_FLAG_DIRTY 0x1
#define DUMP_FLAG_NON_DIRTY 0x2
#define DUMP_FLAG_ALL (DUMP_FLAG_DIRTY|DUMP_FLAG_NON_DIRTY)

#ifndef likely
#if __GCC_MAJOR >= 3
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif
#endif

#define GET_VALUE(x, t)  do {               \
    if(unlikely(in_uiOffset + sizeof(t) > all_size))    \
    goto ERROR_RET;         \
    x = (typeof(x))*(t *)(in_pchContent + in_uiOffset);\
    in_uiOffset += sizeof(t);       \
} while(0)

#define SKIP_SIZE(s)    do {            \
    if(unlikely(in_uiOffset + s > all_size))    \
    goto ERROR_RET;     \
    in_uiOffset += s;       \
} while(0)

static int gLoadRatio;
static int gFlag;
static char gCacheCfg[256];
static char gTabCfg[256];
static char gSource[256];
static char gDest[256];
static char binlog_backup_dir_or_file[256];

CConfig *gConfig;
CDbConfig* dbConfig;
CTableDefinition *gTableDef;
cReaderInterface* gReader;
cWriterInterface* gWriter;
int targetNewHash;
int hashChanging;

static int disableCache;
static int cacheKey;
static int asyncUpdate;
static int gLimitBegin;
static int gLimitEnd;
static int gIgnoreError;
CB::cb_reader< CB::cb_decoder >  _logr;
int     _continous_error_count;
uint64_t    _recovery_node_count;

#define PRINT(fmt, args...) printf(fmt"\n", ##args)


int DecodeKey(CRowValue &stRow, char * in_pchContent, int & in_uiOffset,unsigned int all_size, int keynum)
{
    for (int j = 0; j < keynum; j++)// 解析key
    {
        switch(stRow.FieldType(j))
        {
        case DField::Signed:
            if(unlikely(stRow.FieldSize(j) > (int)sizeof(int32_t) ))
            {
                GET_VALUE(stRow.FieldValue(j)->s64, int64_t);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->s64, int32_t);

            }
            break;

        case DField::Unsigned:
            if(unlikely(stRow.FieldSize(j) > (int)sizeof(uint32_t) ))
            {
                GET_VALUE(stRow.FieldValue(j)->u64, uint64_t);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->u64, uint32_t);
            }
            break;

        case DField::Float:         //浮点数
            if(likely(stRow.FieldSize(j) > (int)sizeof(float) ))
            {
                GET_VALUE(stRow.FieldValue(j)->flt, double);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->flt, float);
            }
            break;

        case DField::String:        //字符串
        case DField::Binary:        //二进制数据
        default:
            {
                GET_VALUE(stRow.FieldValue(j)->bin.len, int);
                stRow.FieldValue(j)->bin.ptr = in_pchContent + in_uiOffset;
                SKIP_SIZE((unsigned int)stRow.FieldValue(j)->bin.len);
                break;
            }
        }//end of switch
    }

    return(0);

ERROR_RET:
    return(-100);
}

int DecodeRow(CRowValue &stRow,char * in_pchContent, int & in_uiOffset,unsigned int all_size, int in_uchKeyIdx)
{

    
    in_uiOffset += sizeof(unsigned char);
    for (int j = in_uchKeyIdx+1; j <= stRow.NumFields() ; j++)//拷贝一行数据
    {
        switch(stRow.FieldType(j))
        {
        case DField::Signed:
            if(unlikely(stRow.FieldSize(j) > (int)sizeof(int32_t) ))
            {
                GET_VALUE(stRow.FieldValue(j)->s64, int64_t);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->s64, int32_t);
            }
            break;

        case DField::Unsigned:
            if(unlikely(stRow.FieldSize(j) > (int)sizeof(uint32_t) ))
            {
                GET_VALUE(stRow.FieldValue(j)->u64, uint64_t);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->u64, uint32_t);
            }
            break;

        case DField::Float:         //浮点数
            if(likely(stRow.FieldSize(j) > (int)sizeof(float) ))
            {
                GET_VALUE(stRow.FieldValue(j)->flt, double);
            }
            else
            {
                GET_VALUE(stRow.FieldValue(j)->flt, float);
            }
            break;

        case DField::String:        //字符串
        case DField::Binary:        //二进制数据
        default:
            {
                GET_VALUE(stRow.FieldValue(j)->bin.len, int);
                stRow.FieldValue(j)->bin.ptr = in_pchContent + in_uiOffset;
                SKIP_SIZE((unsigned int)stRow.FieldValue(j)->bin.len);
                break;
            }
        }//end of switch
    }

    return(0);

ERROR_RET:
    return(-100);
}

void ShowUsage(const char* szName)
{
	printf("usage: %s -f cache_conf_file -t table_conf_file -s data_soruce -d data_dest -b bin_log_file  [-r ratio] [-g flag] [-i 0|1]\n", szName);
	printf("	-f cache_conf_file\n");
	printf("		ttc's cache.conf path\n");
	printf("	-t table_conf_file\n");
	printf("		ttc's table.conf path\n");
	printf("	-s data_soruce\n");
	printf("		data source, db|mem|txt_file_path|bin_log\n");
	printf("	-b bin_log_file\n");
	printf("		when conver bin_log to txt,this is the dir of bin_log_file\n");
	printf("	-d data_dest\n");
	printf("		output destination, db|mem|txt_file_path\n");
	printf("	-r ratio\n");
	printf("		how many data load into cache[1-100]\n");
	printf("	-g flag\n");
	printf("		dump all data or dirty data only, in case [-s mem] only\n");
	printf("	-g 0|1\n");
	printf("		ignore read error or not\n");
	
	return;
}

int open(const char *backdir)
{
    if(_logr.open(backdir)) 
    {
        PRINT("%s", _logr.error_message());
        return -1;
    }

    return 0;
}

int InitConf(const char* szCacheCfg, const char* szTabCfg)
{
	gConfig = new CConfig;
	if(gConfig->ParseConfig(szCacheCfg, "cache")){
		fprintf(stderr, "parse cache config error.\n");
		return -1;
	}
	
	dbConfig = CDbConfig::Load(szTabCfg);
	if(dbConfig == NULL){
		fprintf(stderr, "load table configuire file error.\n");
		return -2;
	}
	
	gTableDef = dbConfig->BuildTableDefinition();
	if(gTableDef == NULL){
		fprintf(stderr, "build table definition error.\n");
		return -3;
	}
	
	return 0;
}


int InitCacheReader(cCacheReader*& pstCacheReader)
{
	int iRet;
	
	const char *keyStr = gConfig->GetStrVal("cache", "CacheShmKey");
	if(keyStr==NULL){
		cacheKey = 0;
		log_notice("CacheShmKey not set!");
		return -1;
	}
	else if(!strcasecmp(keyStr, "none")){
		log_crit("CacheShmKey set to NONE, Cache disabled");
		disableCache = 1;
		return -1;
	}
	else if(isdigit(keyStr[0]))
	{
		cacheKey = strtol(keyStr, NULL, 0);
	} else {
		log_crit("Invalid CacheShmKey value \"%s\"", keyStr);
		return -1;
	}
	
	pstCacheReader = new cCacheReader;
	if(pstCacheReader == NULL){
		log_crit("new cache-reader error: %m\n");
		return(-1);
	}
	
	iRet = pstCacheReader->cache_open(cacheKey, gTableDef->KeyFormat(), gTableDef);
	if(iRet != 0){
		log_crit("cache open error: %d, %s\n", iRet, pstCacheReader->err_msg());
		return(-2);
	}
	
	return 0;
}

int InitCacheWriter(cCacheWriter*& pstCacheWriter)
{
	int iRet;
	CacheInfo stCacheInfo;
	
	memset(&stCacheInfo, 0, sizeof(stCacheInfo));
	
	pstCacheWriter = new cCacheWriter;
	if(pstCacheWriter == NULL){
		log_crit("new cache-writer error: %m\n");
		return -1;
	}
	
	// read config
	asyncUpdate = gConfig->GetIntVal("cache", "DelayUpdate", 0);
	if(asyncUpdate < 0 || asyncUpdate > 1){
		log_crit("Invalid DelayUpdate value");
		return -1;
	}

	const char *keyStr = gConfig->GetStrVal("cache", "CacheShmKey");
	if(keyStr==NULL){
		cacheKey = 0;
		log_notice("CacheShmKey not set!");
		return -1;
	}
	else if(!strcasecmp(keyStr, "none")){
		log_crit("CacheShmKey set to NONE, Cache disabled");
		disableCache = 1;
		return -1;
	}
	else if(isdigit(keyStr[0]))
	{
		cacheKey = strtol(keyStr, NULL, 0);
	} else {
		log_crit("Invalid CacheShmKey value \"%s\"", keyStr);
		return -1;
	}
	
	unsigned long long cacheSize = gConfig->GetSizeVal("cache", "CacheMemorySize", 0, 'M');
	if(cacheSize == 0){
		log_crit("CacheMemorySize not found in cache.conf");
		return -1;
	}
	else if(sizeof(long)==4 && cacheSize >= 4000000000ULL)
	{
		log_crit("CacheMemorySize %lld too large", cacheSize);
		return -1;
	} else {
		stCacheInfo.Init(
				gTableDef->KeyFormat(),
				cacheSize,
				gConfig->GetIntVal("cache", "CacheShmVersion", 4)
				);
	}

	stCacheInfo.keySize = gTableDef->KeyFormat();
	stCacheInfo.ipcMemKey = cacheKey;
	stCacheInfo.syncUpdate = !asyncUpdate;	
	stCacheInfo.createOnly = 1;
	
	// cache open
	iRet = pstCacheWriter->cache_open(&stCacheInfo, gTableDef);
	if(iRet != 0){
		log_crit("cache open error: %d, %s\n", iRet, pstCacheWriter->err_msg());
		return -2;
	}
	
	return 0;
}

int ValueEqual(const CValue& left, const CValue& right, int FieldType)
{
	switch(FieldType)
    {
		case DField::Signed:
			return left.s64==right.s64;
			
		case DField::Unsigned:
			return left.u64==right.u64;
			
		case DField::Float:
			return left.flt==right.flt;
			
		case DField::String:
			return StringEqual(left, right);
				
		case DField::Binary:
			return BinaryEqual(left, right);
			
		default:
			return 0;
	}
	
	return 0;
}

int Process(cReaderInterface* pstReader, cWriterInterface* pstWriter)
{
	int iRet;
	int iNodeRowCnt;
	char achKey[MAX_KEY_LEN];
	CRowValue* pstPreRow;
	CRowValue* pstRow;
	
	pstRow = new CRowValue(gTableDef);
	if(pstRow == NULL){
		fprintf(stderr, "new row error: %m\n");
		return -1;
	}
	
	if(pstWriter->begin_write() != 0){
		log_error("writer begin write error: %s", pstWriter->err_msg());
		return -2;
	}
	pstReader->begin_read();
	
	iNodeRowCnt = 0;
	pstPreRow = NULL;
	while(!pstWriter->full() && !pstReader->end()){
		iRet = pstReader->read_row(*pstRow);
		if(iRet != 0){
			if(pstReader->end())
				break;
			
			if(gIgnoreError && strcasecmp(gSource, "mem") == 0){
				cCacheReader* pstCacheReader = (cCacheReader*)pstReader;
				pstCacheReader->fetch_node();
				continue;
			}
			
			log_error("read from txt error: %s", pstReader->err_msg());
			if(pstRow != NULL)
				delete pstRow;
			if(pstPreRow != NULL)
				delete pstPreRow;
			return -3;
		}
		
		// parse row
		if(strcasecmp(gSource, "mem") == 0){
			cCacheReader* pstCacheReader = (cCacheReader*)pstReader;
			int iRowFlag = pstCacheReader->row_flag_dirty()?DUMP_FLAG_DIRTY:DUMP_FLAG_NON_DIRTY;
			if(!(gFlag & iRowFlag)){
				continue;
			}
		}
		
		if(pstPreRow != NULL){
			for(int i=0; i<gTableDef->KeyFields(); i++){
				if(!ValueEqual((*pstPreRow)[i], (*pstRow)[i], pstPreRow->FieldType(i))){ // another node
					if(pstWriter->commit_node() != 0 && !pstWriter->full()){
						log_error("writer commit node error: %s", pstWriter->err_msg());
						if(pstRow != NULL)
							delete pstRow;
						if(pstPreRow != NULL)
							delete pstPreRow;
						return -4;
					}
					delete pstPreRow;
					pstPreRow = NULL;
					
					if(gLoadRatio != 100){ // 不是100% cache
						float fLimit;
						cDBReader* pstDBReader=(cDBReader*)pstReader;
						fLimit = pstDBReader->CurTabRowNum()*0.01f*gLoadRatio;
						if(pstDBReader->CurRowIdx() >= (int)fLimit)
							pstDBReader->FetchTable();
					}
					iNodeRowCnt = 0;
					
					break;
				}
			}
		}
		if(pstPreRow == NULL){
			pstPreRow = new CRowValue(*pstRow);
			if(pstPreRow->FieldType(0) == DField::String || pstPreRow->FieldType(0) == DField::Binary){ 
				memcpy(achKey, (*pstPreRow)[0].str.ptr, (*pstPreRow)[0].str.len); // 字符串key在new的时候只是给指针赋值，现在copy一份
				pstPreRow->FieldValue(0)->str.ptr = achKey;
			}
		}
		
		if(iNodeRowCnt < gLimitBegin) // limit
			continue;
		if(gLimitEnd!=-1 && iNodeRowCnt >= gLimitBegin+gLimitEnd)
			continue;
		iRet = pstWriter->write_row(*pstRow);
		if(iRet != 0){
			log_error("writer write row error: %s", pstWriter->err_msg());
			if(pstRow != NULL)
				delete pstRow;
			if(pstPreRow != NULL)
				delete pstPreRow;
			return -5;
		}
		iNodeRowCnt++;
	}
	if(!pstWriter->full()){ // 最后一个node肯定没有commit
		pstWriter->commit_node();
	}
	
	if(pstRow != NULL)
		delete pstRow;
	if(pstPreRow != NULL)
		delete pstPreRow;
	
	log_info("process [%s] to [%s] success.", gSource, gDest);	

	return 0;
}



int cover (cWriterInterface* pstWriter)
{
    int ret = 0;
    int iNodeRowCnt;
    CRowValue* pstRow;

    pstRow = new CRowValue(gTableDef);
    if(pstRow == NULL)
    {
        fprintf(stderr, "new row error: %m\n");
        return -1;
    }

    if(pstWriter->begin_write() != 0)
    {
        log_error("writer begin write error: %s", pstWriter->err_msg());
        return -2;
    }
    //read binlog 

    iNodeRowCnt = 0;
    while(!pstWriter->full())
    {
        //add by caro
        

        if(_continous_error_count > 10)
        {
            PRINT("continous error count exceeds threshold(10), shutdown");
            return -1;
        }

        ret = _logr.read();

        if (ret > 0) //one key 
        {
            if(_logr.decode() || _logr.size() != 2) 
            {
                PRINT("data maybe corrupted, skip this record");
                ++_continous_error_count;
                continue;
            }

            CRawFormat rawdata = *(CRawFormat *)_logr[1].ptr;
            unsigned int rownum = rawdata.m_uiRowCnt;
            int offset = 9 + _logr[0].len;
            int keyoff = 0;
            DecodeKey(*pstRow,_logr[0].ptr,keyoff,_logr[0].len,gTableDef->KeyFields()); 
            for(unsigned int i = 0; i< rownum;i++)//对一个key，读取它的每一行节点
            {
                DecodeRow(*pstRow,_logr[1].ptr,offset,rawdata.m_uiDataSize,gTableDef->KeyFields()-1);
                pstWriter->write_row(*pstRow); 
            }
            _recovery_node_count++;            
        }
        else if ( ret == 0) // end
        {
            PRINT("recovery "UINT64FMT" node", _recovery_node_count);
           // PRINT("cold recovery completed at %s", get_current_time_str());
            return 0;
        }
        else//error
        {
            PRINT("logr.read error: %s", _logr.error_message());
            ++_continous_error_count;
        }
    }
    if(pstRow != NULL)
        delete pstRow;

    log_info("process [%s] to [%s] success.", gSource, gDest);	

    return 0;

}

int Init()
{
	int iRet;
	
	// reader
	if(strcasecmp(gSource, "mem") == 0){
		cCacheReader* pstCacheReader=NULL;
		iRet = InitCacheReader(pstCacheReader);
		if(iRet != 0)
			return -1;
		gReader = pstCacheReader;
	}
	else if(strcasecmp(gSource, "db") == 0){
		cDBReader* pstDBReader = new cDBReader;
		if(pstDBReader->Init(gTabCfg) != 0){
			log_crit("init db reader error: %s", pstDBReader->err_msg());
			return -2;
		}
		gReader = pstDBReader;
	}
	else{
		cTxtReader* pstTxtReader = new cTxtReader;
		if(pstTxtReader->Open(gSource) != 0){
			log_crit("init txt reader error: %s", pstTxtReader->err_msg());
			return -3;
		}
		gReader = pstTxtReader;
	}
	
	// writer
	if(strcasecmp(gDest, "mem") == 0){
		cCacheWriter* pstCacheWriter=NULL;
		iRet = InitCacheWriter(pstCacheWriter);
		if(iRet != 0)
			return -4;
		gWriter = pstCacheWriter;
	}
	else if(strcasecmp(gDest, "db") == 0){
		cDBWriter* pstDBWriter = new cDBWriter;
		if(pstDBWriter == NULL){
			log_crit("new db writer error: %m");
			return -5;
		}
		if(pstDBWriter->Init(gTabCfg) != 0){
			log_crit("db writer init error: %s\n", pstDBWriter->err_msg());
			return -6;
		}
		gWriter = pstDBWriter;
	}
	else{
		cTxtWriter* pstTxtWriter = new cTxtWriter;
		if(pstTxtWriter == NULL){
			log_crit("new txt writer error: %m");
			return -7;
		}
		if(pstTxtWriter->Open(gDest, "w") != 0){
			log_crit("txt writer open error: %s\n", pstTxtWriter->err_msg());
			return -8;
		}
		gWriter = pstTxtWriter;
	}
	
	return 0;
}

int main(int argc, char **argv)
{
	int iRet;
	int iOpt;
	
	gIgnoreError = 0;
	gLoadRatio = 100;
	gFlag = DUMP_FLAG_ALL;
	memset(gSource, 0, sizeof(gSource));
	memset(gDest, 0, sizeof(gDest));
	memset(gTabCfg, 0, sizeof(gTabCfg));
	memset(gCacheCfg, 0, sizeof(gCacheCfg));
	while((iOpt = getopt(argc, argv, "f:t:s:d:r:g:i:b:")) != -1){
		switch(iOpt){
			case 'f':
				STRCPY(gCacheCfg, optarg);
				break;
				
			case 't':
				STRCPY(gTabCfg, optarg);
				break;
			
			case 's':
				STRCPY(gSource, optarg);
				break;
			
			case 'd':
				STRCPY(gDest, optarg);
				break;
			
			case 'b':
				STRCPY(binlog_backup_dir_or_file, optarg);
				break;
			
			case 'r':
				gLoadRatio = atoi(optarg);
				break;
			
			case 'g':
				if(strcasecmp("all", optarg) == 0)
					gFlag = DUMP_FLAG_ALL;
				else if(strcasecmp("dirty", optarg) == 0)
					gFlag = DUMP_FLAG_DIRTY;
				break;
			
			case 'i':
				gIgnoreError = (atoi(optarg) != 0)?1:0;
				break;
				
			default:
				fprintf(stderr, "invalid option: %c\n", iOpt);
				ShowUsage(argv[0]);
				exit(1);
		}
	}
	if(gTabCfg[0] == 0 || gCacheCfg[0] == 0 || gSource[0] == 0 || gDest[0] == 0 || gLoadRatio<1 || gLoadRatio>100){
		ShowUsage(argv[0]);
		exit(1);
	}
	if(strcasecmp(gSource, gDest) == 0){
		fprintf(stderr, "data source & data destination is same!\n");
		exit(1);
	}
	
	// read config file
	iRet = InitConf(gCacheCfg, gTabCfg);
	if(iRet != 0)
		exit(2);

	hashChanging = gConfig->GetIntVal("cache", "HashChanging", 0);
	targetNewHash = gConfig->GetIntVal("cache", "TargetNewHash", 0);
	
	_init_log_("mem_tool");
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
	_set_log_thread_name_("mem_tool");
	
	// init db reader
	char* pszDBOrder = dbConfig->ordSql;
	gLimitBegin = 0;
	gLimitEnd = -1;
	if(strcasecmp(gSource, "db")==0 && pszDBOrder != NULL){ // 处理limit的逻辑
		char* p;
		char* pszUpper = strdup(pszDBOrder);
		for(p=pszUpper; *p; p++)
			*p = toupper(*p);
		if((p=strstr(pszUpper, "LIMIT")) != NULL){
			if((p==pszUpper || isspace(*(p-1))) && (*(p+5)==0 || isspace(*(p+5)))){ // 确认LIMIT不是子字符串
				if(sscanf(p+5, "%d%*[ \t,]%d", &gLimitBegin, &gLimitEnd) != 2){ // limit x,y ?
	                gLimitBegin = 0;
	                if(sscanf(p+5, "%d", &gLimitEnd)!=1) // limit x ?
	                        gLimitEnd = -1;
				}
			}
		}
		free(pszUpper);
	}
	
	if(strcasecmp(gSource, "bin_log")==0)//the source is binlog
	{
		if(binlog_backup_dir_or_file[0] == 0) //don't give the bin_log dir
		{
			printf("you want cover bin_log to txt,please give the bin_log_dir\n");
		     ShowUsage(argv[0]);
        }
		open(binlog_backup_dir_or_file);
		cTxtWriter* pstTxtWriter = new cTxtWriter;
		if(pstTxtWriter == NULL){
			log_crit("new txt writer error: %m");
			return -7;
		}
		if(pstTxtWriter->Open(gDest, "w") != 0){
			log_crit("txt writer open error: %s\n", pstTxtWriter->err_msg());
			return -8;
		}
		gWriter = pstTxtWriter;
		cover(gWriter);
		
	}
	else
	{
		iRet = Init();
		if(iRet != 0)
			exit(6);
	
		iRet = Process(gReader, gWriter);
	}
	delete gReader;
	delete gWriter;
	delete gConfig;
	
	return(0);
}

