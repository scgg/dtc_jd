#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include "log.h"
#include "json/json.h"
#include "plugin.h"
#include "http_plugin_request_msg.h"
#include "http_plugin_response_msg.h"
#include "plugin_operate.h"
#include <stdio.h>

using namespace TTC;

int handle_init(int argc, char **argv, int threadtype) {
	printf("enter handle_init!!!!!!!!!!");
	_init_log_("plugin", "../log");
	_set_log_level_(7);
	log_info("plugin start!");
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

	//CHttpPluginRequestMsg decoder;
	CHttpPluginResponseMsg encoder;
	encoder.SetCode(200);
	encoder.SetHttpVersion(1);
	encoder.SetKeepAlive(false);

	Json::FastWriter writer;
	Json::Value packet;

	packet["retCode"] = "-1";
	packet["Msg"] = "ok!";

	std::string respBody = writer.write(packet);
	encoder.SetBody(respBody);
	std::string response = encoder.Encode();
	char *send = new char[response.size()];
	memcpy(send, response.c_str(), response.size());
	*send_buf = send;
	*send_len = response.size();

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
