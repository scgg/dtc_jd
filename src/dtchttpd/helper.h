#ifndef HELPER_H
#define HELPER_H

#include <time.h>
#include <sys/time.h>
#include <string>
#include <stdio.h>
#include "curl_http.h"
#include "json/json.h"
#include "log.h"
#include <ext/hash_set>

static inline std::string GetCurrentFormatTime()
{
	time_t t = time(0);
    struct tm* tm_time = localtime(&t);

    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d",
        1900 + tm_time->tm_year, 1 + tm_time->tm_mon, tm_time->tm_mday,
        tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);

    return std::string(buf);
}

static inline long GetUnixTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long)tv.tv_sec;
}

static inline int DoHttpRequest(const std::string url, const std::string content)
{
	//do http post
	CurlHttp curl_http;
	curl_http.SetTimeout(10);
	curl_http.SetHttpParams("%s", content.c_str());
	BuffV buf;
	int ret = curl_http.HttpRequest(url, &buf, false);
	log_info("buf is %s", buf.Ptr());	
	if (0 != ret)	
	{
		log_error("call url[%s] failed. ret is %d content is %s", url.c_str(), ret, content.c_str());
		return -1;
	}
	//parse result
	std::string strRsp = buf.Ptr();
	Json::Reader reader;
	Json::Value root;
	ret = reader.parse(strRsp, root, false);
	if (0 == ret)
	{
		log_error("parse json failed, ret is %d response is %s content is %s", ret, strRsp.c_str(), content.c_str());
		return -1;
	}
	if ("1" != root["retCode"].asString())
	{
		log_error("return code is not 1, retCode is %s response is %s content is %s", root["retCode"].asString().c_str(), strRsp.c_str(), content.c_str());
		return -1;
	}
	return 0;
}

static inline __gnu_cxx::hash_set<int> SplitString(std::string bid_str)
{
	__gnu_cxx::hash_set<int> bid_set;
	std::string::size_type start = 0, end;
	int front_one;
	while ((end = bid_str.find(',', start)) != std::string::npos)
	{
		front_one = atoi(bid_str.substr(start, end-start).c_str());
		bid_set.insert(front_one);
		start = end + 1;
	}
	int last_one = atoi(bid_str.substr(start).c_str());
	bid_set.insert(last_one);
	return bid_set;
}

#endif
