#include "session.h"
#include <stdlib.h>

int Session::HandlePkg(void *pPkg, int iPkgLen)
{
	log_info("handle package:%s pkglen:%d", (char *)pPkg, iPkgLen);
	CHttpAgentRequestMsg decoder;
	if (decoder.Decode((char *)pPkg, iPkgLen) < 0)
	{
		CloseConnection();
		log_error("Decode Error");
	}
	else
	{
		//process message
		int res = MsgParser::Parse(decoder.GetBody());
		//get response content
		std::string respBody;
		if (0 == res)
		{
			respBody = "{\"retCode\":\"1\"}";
		}
		else
		{
			respBody = "{\"retCode\":\"0\",\"retMessage\":\"process fail\"}";
		}
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
