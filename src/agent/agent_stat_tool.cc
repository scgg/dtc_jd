#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>

#include "log.h"
#include "StatClient.h"
#include "formater.h"
#include "tinyxml.h"
#include "config.h"
#include "config_file.h"
#include "stat_definition.h"
#include "curl_http.h"
#include "json/json.h"

#if __WORDSIZE >= 64
#define F64	"%ld"
#else
#define F64	"%lld"
#endif

#define REPORTNUM 7
#define STATIDX "../stat/agent.%d.stat.idx"
#define DEFNUM (sizeof(gStatReportDefinition)/sizeof(TStatDefinition))

std::vector<CStatClient::iterator> idset;
CTableFormater out;
int outfmt = CTableFormater::FORMAT_ALIGNED;
unsigned char outnoname;
unsigned char rawdata;
unsigned char alldata;
unsigned char nobase;
char gConfig[256] = "../conf/agent.xml";
char gConfFilename[256] = "../conf/agent.conf";

typedef struct _ReportStatInfo
{
	uint32_t uId;
	uint32_t uGetCount;
	uint32_t uReqElapse;
	uint32_t uReqs;
	uint32_t uResp;
	uint32_t uReqPkgSize;
	uint32_t uRespPkgSize;
	uint32_t uGetHit;
	uint32_t uPkgEnterCnt;
	uint32_t uPkgLeaveCnt;
	uint32_t uFrontConnCount;
}ReportStatInfo;

typedef struct _ReportParam
{
	uint32_t uAccessKey;
	uint32_t uCType;
	uint32_t uEType;
	int64_t iData1;
	int64_t iData2;
	int64_t iExtra;
	int64_t iDateTime;
	uint32_t uCmd;
}ReportParam;

enum 
{
	REQ_ELAPSE = 1,
	GET_HIT_RATIO,
	REQ_COUNT,
	NET_IN_FLOW,
	NET_OUT_FLOW = 6,
	RESP_COUNT,
	CONN_COUNT =100
};//CType

enum 
{
	FIVE_SEC = 1,
	TEN_SEC,
	THIRTY_SEC,
	ONE_MIN,
};//EType

enum
{
	CMD_INSERT = 0,
	CMD_UPDATE,
};//CMD

TStatDefinition gStatReportDefinition[] =
{
    AGENT_STAT_REPORT_DEF,
    {0}
};

void usage(void)
{
	fprintf(stderr,
		"Usage: agentstattool [-nct] cmd [args...]\n"
		"options list:\n"
		"	-a  output hidden id too\n"
		"	-r  output unformatted data\n"
		"	-n  Don't output stat name\n"
		"	-t  use tab seperated format\n"
		"	-c  use [,] seperated format\n"
		"command list:\n"
		"        dump [id|id-id]...\n"
		"        reporter [id|id-id]...\n"
	);
	exit(-2);
}

static inline void cell_id(unsigned int id)
{
	out.Cell("%u", id);
}

static inline void cell_sample_id(unsigned int id, unsigned int n)
{
	if(outnoname)
		out.Cell("%u.%u", id, n);
	else
		out.Cell("%u", id);
}

static inline void cell_name(const char *name)
{
	if(outnoname) return;
	if(outfmt == CTableFormater::FORMAT_ALIGNED)
		out.Cell("%s:", name);
	else
		out.Cell("%s", name);
}

static inline void cell_dummy(void)
{
	if(outfmt == CTableFormater::FORMAT_ALIGNED)
		out.Cell("-");
	else
#if GCC_MAJOR < 3
		out.Cell(" ");
#else
		out.Cell(NULL);
#endif
}

static inline void cell_base(int64_t v)
{
	if(outnoname) return;
	if(outfmt == CTableFormater::FORMAT_ALIGNED)
		out.Cell("count[>="F64"]:", v);
	else
		out.Cell("count[>="F64"]", v);
}

static inline void cell_nbase(int v)
{
	if(outfmt == CTableFormater::FORMAT_ALIGNED)
		out.Cell("[%d]", v);
	else
		out.Cell("%d", v);
}

static inline void cell_int(int64_t val)
{
	out.Cell(F64, val);
}

static inline void cell_fixed(int64_t val, int n, int div)
{
	const char *sign = "";
	if(val < 0)
	{
		val = - val;
		sign = "-";
	}
		
	out.Cell("%s"F64".%0*d", sign, val/div, n, (int)(val%div));
}

static inline void cell_percent(int64_t val)
{
	out.Cell(F64"%%", val);
}

static inline void cell_percent_fixed(int64_t val, int n, int div)
{
	const char *sign = "";
	if(val < 0)
	{
		val = - val;
		sign = "-";
	}
		
	out.Cell("%s"F64".%0*d%%", sign, val/div, n, (int)(val%div));
}

static inline void cell_hms(int64_t val)
{
	const char *sign = "";
	if(val < 0)
	{
		val = - val;
		sign = "-";
	}
		
	if(val < 60)
		out.Cell("%s%d", sign, (int)val);
	else if(val < 60*60)
		out.Cell("%s%d:%02d", sign, (int)(val/60), (int)(val%60));
	else
		out.Cell("%s"F64":%02d:%02d", sign, val/3600, (int)((val/60)%60), (int)(val%60));
}

static inline void cell_hmsmsec(int64_t val)
{
	const char *sign = "";
	if(val < 0)
	{
		val = - val;
		sign = "-";
	}
	if(val < 60*1000)
		out.Cell("%s%d.%03d", sign, (int)(val/1000), (int)(val%1000));
	else if(val < 60*60*1000)
		out.Cell("%s%d:%02d.%03d", sign, (int)(val/60000), (int)((val/1000)%60), (int)(val%1000));
	else
		out.Cell("%s"F64":%02d:%02d.%03d", sign,
				val/3600000, (int)((val/60000)%60), (int)((val/1000)%60), (int)(val%1000));
}

static inline void cell_hmsusec(int64_t val)
{
	const char *sign = "";
	if(val < 0)
	{
		val = - val;
		sign = "-";
	}
	out.Cell("%s"F64".%06d", sign, val/1000000, (int)(val%1000000));
}

static inline void cell_datetime(int64_t v)
{
	if(v==0) {
		out.CellV("-");
		return;
	}
	time_t t = v;
	struct tm tm;
	localtime_r(&t, &tm);
	out.CellV("%d-%d-%d %d:%02d:%02d",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static inline void cell_date(int64_t v)
{
	if(v==0) {
		out.CellV("-");
		return;
	}
	time_t t = v;
	struct tm tm;
	localtime_r(&t, &tm);
	out.CellV("%d-%d-%d",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
}

static inline void cell_version(int64_t v)
{
	out.CellV("%d.%d.%d", (int)v/10000, (int)v/100%100, (int)v%100);
}

static inline void cell_bool(int64_t v)
{
        if(0 == v)
        {
                out.CellV("NO");
        }
        else
        {
                out.CellV("YES");
        }
}

inline void row_init(void)
{
	out.NewRow();
}

inline void row_clear(void)
{
	out.ClearRow();
}

inline void dump_table(void)
{
	out.Dump(stdout, outfmt);
}

void cell_value(int unit, int64_t val)
{
	if(rawdata) unit = SU_INT;
	switch(unit)
	{
	default:
	case SU_HIDE:
	case SU_INT: cell_int(val); break;
	case SU_INT_1: cell_fixed(val, 1, 10); break;
	case SU_INT_2: cell_fixed(val, 2, 100); break;
	case SU_INT_3: cell_fixed(val, 3, 1000); break;
	case SU_INT_4: cell_fixed(val, 4, 10000); break;
	case SU_INT_5: cell_fixed(val, 5, 100000); break;
	case SU_INT_6: cell_fixed(val, 6, 1000000); break;
	case SU_MSEC: cell_hmsmsec(val); break;
	case SU_USEC: cell_hmsusec(val); break;
	case SU_TIME: cell_hms(val); break;
	case SU_DATE: cell_date(val); break;
	case SU_DATETIME: cell_datetime(val); break;
	case SU_VERSION: cell_version(val); break;
        case SU_BOOL: cell_bool(val); break;
	case SU_PERCENT: cell_percent(val); break;
	case SU_PERCENT_1: cell_percent_fixed(val, 1, 10); break;
	case SU_PERCENT_2: cell_percent_fixed(val, 2, 100); break;
	case SU_PERCENT_3: cell_percent_fixed(val, 3, 1000); break;
	}
}

void dump_data(CStatClient &stc)
{
	int64_t sc[16];
	int sn;

	unsigned int i;
	CStatClient::iterator s;

	stc.CheckPoint();

	for(i = 0; i < idset.size(); i++)
	{
		s = idset[i];

		if(alldata==0 && s->unit() == SU_HIDE)
			continue;

		row_init();
		switch(s->type())
		{
		case SA_SAMPLE:
			cell_id(s->id());
			cell_name(s->name());
			cell_value(s->unit(), stc.ReadSampleAverage(s, SC_CUR));
			cell_value(s->unit(), stc.ReadSampleAverage(s, SCC_10S));
			cell_value(s->unit(), stc.ReadSampleAverage(s, SCC_10M));
			cell_value(s->unit(), stc.ReadSampleAverage(s, SCC_ALL));

			if(nobase<2)
			{
				row_init();
				cell_sample_id(s->id(), 1);
				cell_name("count[all]");
				cell_value(SU_INT, stc.ReadSampleCounter(s, SC_CUR));
				cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_10S));
				cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_10M));
				cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_ALL));
			}
			if(nobase==0)
			{
				sn = stc.GetCountBase(s->id(), sc);
				for(int n=1; n<=sn; n++)
				{
					row_init();
					cell_sample_id(s->id(), n+1);
					cell_base(sc[n-1]);
					cell_value(SU_INT, stc.ReadSampleCounter(s, SC_CUR, n));
					cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_10S, n));
					cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_10M, n));
					cell_value(SU_INT, stc.ReadSampleCounter(s, SCC_ALL, n));
				}
			}

			break;

		case SA_COUNT:
			cell_id(s->id());
			cell_name(s->name());
			cell_value(s->unit(), stc.ReadCounterValue(s, SC_CUR));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10S));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10M));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_ALL));
			break;
		case SA_VALUE:
			cell_id(s->id());
			cell_name(s->name());
			cell_value(s->unit(), stc.ReadCounterValue(s, SC_CUR));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10S));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10M));
			cell_dummy();
			break;

		case SA_CONST:
			cell_id(s->id());
			cell_name(s->name());
			cell_value(s->unit(), stc.ReadCounterValue(s, SC_CUR));
			switch(s->unit())
			{
			case SU_DATETIME:
			case SU_DATE:
			case SU_VERSION:
                        case SU_BOOL:
				break;
			default:
				cell_dummy();
				cell_dummy();
				cell_dummy();
				break;
			}
			break;

		case SA_EXPR:
			cell_id(s->id());
			cell_name(s->name());
			cell_dummy();
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10S));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_10M));
			cell_value(s->unit(), stc.ReadCounterValue(s, SCC_ALL));
			break;

		default:
			row_clear();
		}
	}
	dump_table();
}

void LockReporter(CStatClient &stc)
{
	for(int i=0; i<60; i++)
	{
		if(stc.Lock("report") == 0)
			return;
		if(i==0)
			fprintf(stderr, "Reporter locked, trying 1 minute.\n");
			log_error("Reporter locked!");
		sleep(1);
	}
	fprintf(stderr, "Can't lock reporter. Exiting.\n");
	log_error("Can't lock reporter. Exiting.");
	exit(-1);
}

void SendToMonitor(ReportStatInfo &v)
{
	int bCurlRet;
	int HttpServiceTimeOut = 2;
	std::string strRsp = "";
	CurlHttp curlHttp;
	time_t iCurTime;
	CConfig gConfig;

	const char *f = strdup(gConfFilename);

	if(gConfig.ParseConfig(f, "MONITOR")) {
		fprintf(stderr, "parse config %s failed\n", f);
		exit(-1);
	}
	
	std::string url_monitor = gConfig.GetStrVal("MONITOR", "MonitorReportURL");

	ReportParam reportParam[REPORTNUM];

	iCurTime = time(NULL);

	//�����ʱ
	reportParam[0].uAccessKey = v.uId;
	reportParam[0].uCType = REQ_ELAPSE;
	reportParam[0].uEType = TEN_SEC;
	reportParam[0].iData1 = v.uReqElapse;
	reportParam[0].iData2 = v.uReqs;
	reportParam[0].iDateTime = iCurTime;
	reportParam[0].uCmd = CMD_INSERT;

	//������
	reportParam[1].uAccessKey = v.uId;
	reportParam[1].uCType = GET_HIT_RATIO;
	reportParam[1].uEType = TEN_SEC;
	reportParam[1].iData1 = v.uGetHit;
	reportParam[1].iData2 = v.uGetCount;
	reportParam[1].iDateTime = iCurTime;
	reportParam[1].uCmd = CMD_INSERT;

	//�������
	reportParam[2].uAccessKey = v.uId;
	reportParam[2].uCType = REQ_COUNT;
	reportParam[2].uEType = TEN_SEC;
	reportParam[2].iData1 = v.uReqs;
	reportParam[2].iData2 = 0;
	reportParam[2].iDateTime = iCurTime;
	reportParam[2].uCmd = CMD_INSERT;

	//�ظ�����
	reportParam[3].uAccessKey = v.uId;
	reportParam[3].uCType = RESP_COUNT;
	reportParam[3].uEType = TEN_SEC;
	reportParam[3].iData1 = v.uResp;
	reportParam[3].iData2 = 0;
	reportParam[3].iDateTime = iCurTime;
	reportParam[3].uCmd = CMD_INSERT;

	//����������
	reportParam[4].uAccessKey = v.uId;
	reportParam[4].uCType = NET_IN_FLOW;
	reportParam[4].uEType = TEN_SEC;
	reportParam[4].iData1 = v.uReqPkgSize;
	reportParam[4].iData2 = v.uPkgEnterCnt;
	reportParam[4].iDateTime = iCurTime;
	reportParam[4].uCmd = CMD_INSERT;

	//���������
	reportParam[5].uAccessKey = v.uId;
	reportParam[5].uCType = NET_OUT_FLOW;
	reportParam[5].uEType = TEN_SEC;
	reportParam[5].iData1 = v.uRespPkgSize;
	reportParam[5].iData2 = v.uPkgLeaveCnt;
	reportParam[5].iDateTime = iCurTime;
	reportParam[5].uCmd = CMD_INSERT;
	
	//连接数上报
	reportParam[6].uAccessKey = v.uId;
	reportParam[6].uCType = CONN_COUNT;
	reportParam[6].uEType = TEN_SEC;
	reportParam[6].iData1 = v.uFrontConnCount;
	reportParam[6].iData2 = 0;
	reportParam[6].iDateTime = iCurTime;
	reportParam[6].uCmd = CMD_UPDATE;

	Json::FastWriter writer;
	Json::Value body;
	body["cmd"] = 3;

	Json::Value statics;
	statics["bid"] = Json::Value(v.uId);
	for(uint32_t i = 0; i < REPORTNUM; i++)
	{
		if(reportParam[i].uCType != CONN_COUNT
			&& reportParam[i].iData1 == 0)
		{
			log_info("iData1=0!");
			continue;
		}

		Json::Value content;
		//content["bid"] = Json::Value(reportParam[i].uAccessKey);
		content["curve"] = reportParam[i].uCType;
		content["etype"] = reportParam[i].uEType;
		content["data1"] = Json::Value::Int64(reportParam[i].iData1);
		content["data2"] = Json::Value::Int64(reportParam[i].iData2);
		content["extra"] = "";
		content["datetime"] = Json::Value::Int64((reportParam[i].iDateTime/10)*10);
		content["cmd"] = reportParam[i].uCmd;

		statics["content"].append(content);
	}
	body["statics"].append(statics);

	BuffV buf;
	std::string strBody = writer.write(body);

	//delete \r\n
	if(strBody[strBody.length()-1]=='\n')
	{
		strBody.erase(strBody.length()-1,1);
	}

	if(strBody[strBody.length()-1]=='\r')
	{
		strBody.erase(strBody.length()-1,1);
	}

	log_info("HttpBody = [%s]", strBody.c_str());

	curlHttp.SetHttpParams("%s", strBody.data());
	curlHttp.SetTimeout(HttpServiceTimeOut);

	bCurlRet = curlHttp.HttpRequest(url_monitor, &buf, false);

	strRsp = buf.Ptr();
	log_info("strRsp:%s", strRsp.c_str());

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(strRsp.c_str(), root))
	{
		log_error("parse Json failed, strRsp:%s", strRsp.c_str());
	}

	std::string strRetCode = root["retCode"].asString();
	log_info("strRetCode:%s", strRetCode.c_str());

	if(strRetCode == "1")
	{
		log_info("call dtchttpd success!");
	}
	else
	{
		log_error("call dtchttpd failed! strRetCode:%s", strRetCode.c_str());
	}
}

int ReportData(CStatClient &stc, uint32_t moduleId)
{
	int iRet = 0;
	int chkpt = 0;
	unsigned int sid = 0;
	int v = 0;
	unsigned int cat = SCC_10S;
	ReportStatInfo reportInfo;

	log_info("Enter ReportData\n");
		
	int c = stc.CheckPoint();
	if(c==chkpt) 
	{
		log_error("CheckPoint c=chkpt=%d, exit!\n", c);
		return 0;
	}
	else
	{
		chkpt = c;
	}

	for(unsigned int i = 0; i < DEFNUM-1; i++)
	{
		sid = gStatReportDefinition[i].id;
		CStatClient::iterator si = stc[(unsigned int)sid];
		
		v = stc.ReadCounterValue(si, cat);

		reportInfo.uId = moduleId;

		switch(sid)
		{
			case GET_COUNT:
				reportInfo.uGetCount = v;
				break;
			case REQUEST_ELAPES:
				reportInfo.uReqElapse = v;
				break;
			case REQUESTS:
				reportInfo.uReqs = v;
				break;
			case RESPONSES:
				reportInfo.uResp = v;
				break;
			case REQPKGSIZE:
				reportInfo.uReqPkgSize = v;
				break;
			case RSPPKGSIZE:
				reportInfo.uRespPkgSize = v;
				break;
			case GET_HIT:
				reportInfo.uGetHit = v;
				break;
			case PKG_ENTER_COUNT:
				reportInfo.uPkgEnterCnt = v;
				break;
			case PKG_LEAVE_COUNT:
				reportInfo.uPkgLeaveCnt = v;
				break;
			case FRONT_CONN_COUNT:
				reportInfo.uFrontConnCount = v;
				break;
			default:
				log_error("invalid id, id=%u", sid);
        		break;
		}
	}

	SendToMonitor(reportInfo);
	
	return 0;
}

static void term(int signo)
{
	exit(0);
}

void Reporter(CStatClient &stc, uint32_t moduleId)
{
	int ppid = getppid();
	signal(SIGTERM, term);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	//LockReporter(stc);

	//report to dtc monitor
	ReportData(stc, moduleId);
}

int BuildModuleMgr(TiXmlElement *modules)
{
	int ret;
	uint32_t moduleId;
	char szModuleName[255] = {0};
	char szIndexName[255] = {0};

    TiXmlElement *m = modules->FirstChildElement("MODULE");
    while(m)
    {
    	CStatClient stc;
		
        moduleId = atoi(m->Attribute("Id"));

		snprintf(szModuleName, sizeof(szModuleName) - 1, "agent.%d", moduleId);
		snprintf(szIndexName, sizeof(szIndexName) - 1, STATIDX, moduleId);

		log_info("BuildModuleMgr moduleId:%d\n", moduleId);

		ret = stc.InitStatInfo(szModuleName, szIndexName);
		if(ret  < 0)
		{
			log_error("Cannot Initialize StatInfo: %s\n", stc.ErrorMessage());
			continue;
		}

		Reporter(stc, moduleId);
		
        m = m->NextSiblingElement("MODULE");
    }

    return 0;
}

void init(uint32_t moduleId, CStatClient &stc)
{
	int ret;
	char szModuleName[255];
	char szIndexName[255];
	
	snprintf(szModuleName, sizeof(szModuleName) - 1, "agent.%d", moduleId);
	snprintf(szIndexName, sizeof(szIndexName) - 1, STATIDX, moduleId);

	ret = stc.InitStatInfo(szModuleName, szIndexName);
	if(ret  < 0)
	{
		fprintf(stderr, "Cannot Initialize StatInfo: %s\n", stc.ErrorMessage());
		exit(-1);
	}
}

void RunReporterOnce()
{
	TiXmlDocument configdoc;
    if(!configdoc.LoadFile(gConfig))
    {
        log_error("load %s failed, errmsg:%s, row:%d, col:%d\n", gConfig, configdoc.ErrorDesc(), configdoc.ErrorRow(),
                configdoc.ErrorCol());
		fprintf(stderr,"load %s failed, errmsg:%s, row:%d, col:%d\n", gConfig, configdoc.ErrorDesc(), configdoc.ErrorRow(),
                configdoc.ErrorCol());
        exit(-1);
    }

    TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *agentconf = rootnode->FirstChildElement("AGENT_CONFIG");
	if (!agentconf) {
		log_error("agent conf miss");
		exit(-1);
	}
	int logLevel=atoi(agentconf->Attribute("LogLevel"));
	_set_log_level_(logLevel);
	
    TiXmlElement *modules = rootnode->FirstChildElement("BUSINESS_MODULE");
    if(!modules)
    {
        log_error("empty module");
		fprintf(stderr,"empty module");
        exit(-1);
    }

    if(BuildModuleMgr(modules) < 0)
    {
        log_error("build module mgr failed");
		fprintf(stderr,"build module mgr failed");
        exit(-1);
    }
}

void parse_stat_id(int argc, char **argv, CStatClient &stc)
{
    CStatClient::iterator n;

    for(n = stc.begin(); n != stc.end(); n++)
        idset.push_back(n);
}

int main(int argc, char **argv)
{
	argv++, --argc;

	_init_log_("agent_stat_tool", "../log");
	_set_log_level_(7);

	while(argc > 0 && argv[0][0]=='-')
	{
		const char *p = argv[0];
		char c;
		while((c=*++p))
		{
			switch(c)
			{
			case 'a':
				alldata = 1;
				break;
			case 'b':
				nobase++;
				break;
			case 'r':
				rawdata = 1;
				break;
			case 't':
				outfmt = CTableFormater::FORMAT_TABBED;
				break;
			case 'c':
				outfmt = CTableFormater::FORMAT_COMMA;
				break;
			case 'n':
				outnoname = 1;
				break;
			case 'h':
			case '?':
			default:
				fprintf(stderr, "Unknown options [%c]\n", c);
				usage();
			}
		}
		argv++, --argc;
	}

	if(argc <= 0)
		usage();
	
	else if(!strcasecmp(argv[0], "help"))
		usage();

	else if(!strcasecmp(argv[0], "dump"))
	{
		CStatClient stc;
		
		init(atoi(argv[1]), stc);
		parse_stat_id(--argc, ++argv, stc);
		dump_data(stc);
	}
	
	else if(!strcasecmp(argv[0], "reporter"))
		RunReporterOnce();
	
	else
		usage();
	
	return 0;
}
