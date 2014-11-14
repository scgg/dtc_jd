#include "session.h"
#include <stdlib.h>
#include "worker.h"
#include <sstream>
#include "json/json.h"

using namespace std;

int Session::HandlePkg(void *pPkg, int iPkgLen){
	log_info("handle package:%s pkglen:%d", (char *)pPkg, iPkgLen);
	CHttpAgentRequestMsg decoder;
	int ret = 0;
	if (decoder.Decode((char *)pPkg, iPkgLen) < 0){
		CloseConnection();
		log_error("Decode Error");
	}else{
		//parse request content
		string content;
		content = decoder.GetBody();
		int bid = 0;
		int cmd = 0;
		string ip, port, token, errMsg;
		MsgParser omgpsr;
		Worker oworker;
		string tableconf, tplCodeDir, respBody;
		Json::Value body;
		Json::FastWriter writer;
		// response json : {"retcode":1, "errmsg":"ok", "codeTplDir":"/usr/local/tmp/tpl/"}
		ret = omgpsr.Parse(content, bid, cmd, ip, port, token, errMsg);
		if(0 != ret){
			log_error("HandlePkg parse request json fail. errCode: %d, errMsg: %s", ret, errMsg.c_str());
			body["retcode"] = ret;
			body["errmsg"] = errMsg;
			respBody = writer.write(body);
		}else{
			ret = oworker.GetTableConf(bid, tableconf, errMsg);
			if(0 != ret){
				log_error("HandlePkg GetTableConf fail. errCode: %d, errMsg: %s", ret, errMsg.c_str());
				body["retcode"] = ret;
				body["errmsg"] = errMsg;
				respBody = writer.write(body);
			}else{
				ret = oworker.MakeTplCode(tableconf, cmd, bid, ip,port, token, tplCodeDir, errMsg);
				if(0 != ret){
					log_error("HandlePkg MakeTplCode fail. errCode: %d, errMsg: %s", ret, errMsg.c_str());
					body["retcode"] = ret;
					body["errmsg"] = errMsg;
					respBody = writer.write(body);
				}else{
					body["retcode"] = 1;
					body["errmsg"] = "success";
					body["codeTplDir"] = tplCodeDir;
					respBody = writer.write(body);
				}
			}
		}
		log_info("respBody: %s", respBody.c_str());
		CHttpAgentResponseMsg encoder;
		encoder.SetCode(200);
		encoder.SetHttpVersion(1);
		encoder.SetKeepAlive(false);
		encoder.SetBody(respBody);
		std::string response = encoder.Encode();
		//send response
		char *send = (char*)malloc(response.size()*sizeof(char));
		memcpy(send, response.c_str(), response.size());
		agentSender->AddPacket(send, response.size());
		agentSender->Send();
		if (!decoder.IsKeepAlive())
			CloseConnection();
		else
			AttachTimer(timerList);
	}

	return 0;
}

