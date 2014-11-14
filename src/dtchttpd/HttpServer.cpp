#include "HttpServer.h"
#include <errno.h>
#include <netinet/tcp.h>
#include "MsgParser.h"
#include <stdlib.h>

int CHttpAcceptor::ListenOn(const std::string &addr)
{
	const char * errmsg = NULL;

	errmsg = bindAddr.SetAddress(addr.c_str(), (const char *)NULL);
	if(errmsg)
	{
		log_error("%s %s", errmsg, addr.c_str());
		return -1;
	}

	netfd = bindAddr.CreateSocket();
	if(netfd < 0)
	{
		log_error("acceptor create socket failed: %m, %d", errno);
		return -1;
	}

	int optval = 1;
	setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
	optval = 60;
	setsockopt(netfd, SOL_TCP, TCP_DEFER_ACCEPT, &optval, sizeof(optval));

	if(bindAddr.BindSocket(netfd) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor bind address failed: %m, %d", errno);
		return -1;
	}

	if(bindAddr.SocketType() == SOCK_STREAM && listen(netfd, 255) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor listen failed: %m, %d", errno);
		return -1;
	}

	EnableInput();

	if(AttachPoller() < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor attach listen socket failed");
		return -1;
	}

	return 0;
}

void CHttpAcceptor::InputNotify()
{
	int newfd;
	struct sockaddr peer;
	socklen_t peerSize = sizeof(peer);

	newfd = accept(netfd, (struct sockaddr *)&peer, &peerSize);
	if(newfd < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
			log_notice("accept new client error: %m, %d", errno);
		return;
	}

	CHttpSession *worker = new CHttpSession(pollThreadGroup, newfd);

	if(NULL == worker)
	{
		log_error("no mem new CHttpWorker");
		return;
	}
}

int CHttpSession::HandlePkg(void *pPkg, int iPkgLen)
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
