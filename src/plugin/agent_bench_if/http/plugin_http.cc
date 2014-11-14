#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <stdio.h>

#include "log.h"
#include "json/json.h"
#include "plugin.h"
#include "http_plugin_request_msg.h"
#include "http_plugin_response_msg.h"
#include "ttcint.h"
#include "tabledef.h"
#include "config.h"
#include "httptool.h"

CConfig *gConfig 	= NULL;

int handle_init(int argc, char **argv, int threadtype) {
	if(PROC_MAIN == threadtype) {
		_init_log_("plugin", "../log");
		char* conf = argv[0];
		gConfig = new CConfig;
		if(gConfig->ParseConfig(conf, "plugin")){
			fprintf(stderr,"parse cache config error.\n");
			return -1;
		}
		int logLevel = 7;
		logLevel = gConfig->GetIntVal ("plugin", "logLevel", 7);
		_set_log_level_(logLevel);
		log_debug("plugin start!");
	}
	return 0;
}

int handle_input(const char* recv_buf, int recv_len, const skinfo_t* skinfo_t) {
	char* recvBuf = const_cast<char*>(recv_buf);
	log_debug("recv_buf=\n%s",recvBuf);
	int iPkgLen = 0;
	CHttpPluginRequestMsg decoder;
	decoder.HandleHeader(recvBuf,recv_len,&iPkgLen);
	log_info("iPkgLen=%d",iPkgLen);
	return iPkgLen;
}

int handle_process(char *recv_buf, int real_len, char **send_buf, int *send_len, const skinfo_t* skinfo_t)
{
	log_info("process recv_buf:%s real_len:%d", recv_buf, real_len);

	CHttpPluginRequestMsg decoder;
	std::string body;
	NCServer* server;
	RequestBody reqBody;
	int ret = decoder.Decode(recv_buf, real_len);
	if(ret < 0)
	{
		log_error("decoder error!");
		goto exit0;
	}
	log_info("Body %s Command:%s XForwardedFor:%s isKeepAlive:%d",
					decoder.GetBody().c_str(),
					decoder.GetCommand().c_str(),
					decoder.GetXForwardedFor().c_str(),
					decoder.IsKeepAlive());
	body = decoder.GetBody();
	ret = HttpTool::Parse(body, reqBody);
	if(ret != 0) {
		goto exit0;
	}
	server = HttpTool::getNCServer(reqBody);
	if(NULL == server) {
		goto exit0;
	}
	ret = HttpTool::doRequest(server, reqBody, send_buf, send_len);
	if(ret != 0) {
		goto exit0;
	}
	return 0;

exit0:
	HttpTool::doError(send_buf, send_len);
	return 0;
}

int handle_open(char **, int *, const skinfo_t*) {
	return 0;
}

int handle_close(const skinfo_t*) {
	return 0;
}

void handle_fini(int threadtype) {
	return;
}

int handle_timer_notify(int plugin_timer_interval, void** name) {
	return 0;
}
