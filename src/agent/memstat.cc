#include <stdint.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>

#include "tinyxml.h"
#include "log.h"
#include "ttcapi.h"
#include "curl_http.h"
#include "json/json.h"

char gConfig[256] = "../conf/agent.xml";
std::string gHttpAddress = "http://192.168.213.162:8080";

enum
{
	memCurve = 0
};//curve

inline bool splitString(const std::string &str, const std::string &delim, std::vector<std::string> &vec)
{
	vec.clear();
	if(str.empty()){
		return false;
	}
	if(delim == "\0"){
		vec.push_back(str);
		return true;
	}
	size_t last = 0;
	size_t index = str.find_first_of(delim,last);
	while (index != std::string::npos)
	{
		std::string subStr = str.substr(last, index-last);
		vec.push_back(subStr);
		last = index + delim.size();
		index = str.find_first_of(delim, last);
	}
	if(last < str.size()){
		std::string subStr = str.substr(last,index-last);
		vec.push_back(subStr);
	}
	return true;
}

void parseIP(const std::string &addr, std::string &ip, std::string &port)
{
	std::vector<std::string> vec;
	splitString(addr, ":", vec);
	ip = vec[0];
	std::string str = vec[1];
	std::vector<std::string> vec2;
	splitString(str, "/", vec2);
	port = vec2[0];
}

void SendToMonitor(const uint32_t &bid, const long long totalMem, const long long usingMem)
{
	//const char *filename = "../conf/reporter.cnf";
	std::string address = gHttpAddress;
	std::string url_monitor = address;
	int bCurlRet;
	int HttpServiceTimeOut = 20;
	std::string strRsp = "";
	BuffV buf;

	Json::FastWriter writer;
	Json::Value body;

	body["curve"] = memCurve;
	body["bid"] = bid;
	body["total"] = totalMem;
	body["usemem"] = usingMem;

	std::string strBody = writer.write(body);
	log_info("HttpBody = [%s]", strBody.c_str());

	CurlHttp curlHttp;

	curlHttp.SetHttpParams("%s", strBody.c_str());
	curlHttp.SetTimeout(HttpServiceTimeOut);

	int ret = curlHttp.HttpRequest(url_monitor, &buf, false);
	if(ret != 0) {
		log_error("curlHttp.HttpRequest Error! ret=%d", ret);
	}

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
		//exit(-1);
	}
}

int runCrontab()
{
	_init_log_("memstat", "../log");
	TiXmlDocument configdoc;
	if(!configdoc.LoadFile(gConfig))
	{
		std::cout << "load agent config error! errmsg="<< configdoc.ErrorDesc() << std::endl;
		return -1;
	}
	TiXmlElement *rootnode = configdoc.RootElement();
	TiXmlElement *agentConfig = rootnode->FirstChildElement("AGENT_CONFIG");
	uint32_t logLevel = atoi(agentConfig->Attribute("LogLevel"));
	_set_log_level_(logLevel);

	TiXmlElement *business = rootnode->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *module = business->FirstChildElement("MODULE");
	while(module)
	{
		uint32_t bid = atoi(module->Attribute("Id"));
		std::string accKey = module->Attribute("AccessToken");
		log_debug("[bussiness] id=%d accKey=%s", bid, accKey.c_str());
		TiXmlElement *instance = module->FirstChildElement("CACHEINSTANCE");
		long long totalMem = 0;
		long long usingMem = 0;

		while(instance)
		{
			std::string addr = instance->Attribute("Addr");
			std::string ip,port;
			parseIP(addr, ip, port);
			log_debug("[instance] ip=%s, port=%s", ip.c_str() ,port.c_str());
			const int QUERY_MEM_INFO_CMD = 20;
			TTC::Server dtcserver;
			dtcserver.SetAddress(ip.c_str(), port.c_str());
			dtcserver.SetTimeout(10);
			//dtcserver.SetAccessKey(accKey.c_str());
			/*随便设置一个表和主键*/
			dtcserver.AddKey("name",TTC::KeyTypeString);
			dtcserver.SetTableName("NULL");
			TTC::SvrAdminRequest request(&dtcserver);
			request.SetAdminCode(QUERY_MEM_INFO_CMD);

			TTC::Result result;
			int ret = request.Execute(result);
			if (ret != 0)
			{
				log_error("execute fail, ret=%d, errStep=%s, errMsg:%s", ret, result.ErrorFrom(), result.ErrorMessage());
				instance = instance->NextSiblingElement("CACHEINSTANCE");
				continue;
			}
			totalMem += result.MemSize();
			usingMem += result.DataSize();
			log_debug("[instance] memsize=%lld, datasize=%lld", result.MemSize(), result.DataSize());

			instance = instance->NextSiblingElement("CACHEINSTANCE");
		}
		// TODO:上报
		if(totalMem == 0) {
			log_debug("exit -1");
			exit(-1);
		}
		SendToMonitor(bid, totalMem, usingMem);
		log_debug("[bussiness] totalMem=%lld, usingMem=%lld", totalMem, usingMem);
		module = module->NextSiblingElement("MODULE");
	}

	return 0;
}

int main(int args, char** argv)
{
	if(args==3) {
		std::string cmd = argv[1];
		gHttpAddress = argv[2];
		if(cmd == "-c") { // 以crontab模式执行
			int ret = runCrontab();
		}
	}
	else {
		std::cout << "param error!" << std::endl;
		return -1;
	}
	return 0;

}
