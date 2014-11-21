#ifndef PROBER_HTTPSERVER_H
#define PROBER_HTTPSERVER_H

#include <queue>
#include "poller.h"
#include "sockaddr.h"
#include "poll_thread.h"
#include "stream_receiver.h"
#include "sender.h"
#include "http_agent_request_msg.h"
#include "http_agent_response_msg.h"
#include "prober_config.h"
#include "log.h"
#include "json/json.h"
#include "prober_cmd.h"
#include "RelativeHourCalculator.h"
#include "map_reduce.h"

class CDispatcher;

class CHttpAcceptor : public CPollerObject
{
public:
	CHttpAcceptor(CPollThread *o, CProberConfig *s, CDispatcher **d):CPollerObject(o){
		pollThread = o;
		proberConfig = s;
		dp = d;
	};

	int ListenOn(const std::string &addr);
	virtual void InputNotify();
private:
	CSocketAddress bindAddr;
	CPollThread *pollThread;
	CProberConfig *proberConfig;
	CDispatcher **dp;
};

class CHttpServer
{
public:
	CHttpServer(CPollThread *o, CProberConfig *s, CDispatcher **d){
		pollThread = o;
		httpAcceptor = new CHttpAcceptor(pollThread, s, d);
	}
	int ListenOn(const std::string &addr)
	{
		return httpAcceptor->ListenOn(addr);
	}
	~CHttpServer() {
		delete httpAcceptor;
	}
private:
	CHttpAcceptor *httpAcceptor;
	CPollThread *pollThread;
};

class CHttpSession : public CPollerObject, public CTimerObject, public IReceiver
{
public:
	CHttpSession(CPollThread *o, int fd, CProberConfig *s, CDispatcher **d):CPollerObject(o, fd){
		EnableInput();
		AttachPoller();
		streamReceiver = new CStreamReceiver(fd, 0, this);
		agentSender = new CAGSender(fd, 1000);
		proberConfig = s;
		dp = d;
		targetCmd = s->WorkerNum();
		agentSender->Build();
		timerList = dynamic_cast<CTimerUnit *>(ownerUnit)->GetTimerList(1);
	}

	~CHttpSession()
	{
		log_info("enter");
		if (streamReceiver)
			delete streamReceiver;
		if (agentSender)
			delete agentSender;
	}

	virtual void InputNotify()
	{
		log_info("input notify");
		if (streamReceiver->Recv() < 0)
			CloseConnection();
	}

	virtual void TimerNotify()
	{
		log_info("timer trigger");
		CloseConnection();
	}

	virtual int HandleHeader(void *pPkg, int iBytesRecved, int *piPkgLen)
	{
		log_debug("handle header buffer:%s bufferlen:%d", (char*)pPkg, iBytesRecved);
		CHttpAgentRequestMsg decoder;
		return decoder.HandleHeader(pPkg, iBytesRecved, piPkgLen);
	}

	virtual int HandlePkg(void *pPkg, int iPkgLen);
	virtual bool ProcessBody(std::string reqBody, std::string &respBody); 
	void Complete();
	void ReplyPkg();
	bool Split();
	bool ProcessQueued();
	bool InitMemConf(std::string confBase);

	void CloseConnection()
	{
		DetachPoller();
		//close(netfd);
		delete this;
	}
private:
	std::string        respBody;
	CHttpAgentResponseMsg encoder;
	CHttpAgentRequestMsg decoder;
	CPollThread   *ownerThread;
	CStreamReceiver    *streamReceiver;
	CAGSender          *agentSender;
	CTimerList         *timerList;
	CProberConfig      *proberConfig;
	CDispatcher **dp;
	CProberCmd **cmdArray;
	CProberCmd *proberCmd;
	Json::Value in, out;
	Json::Reader reader;
	Json::FastWriter writer;
	ProberResultSet prs;
	std::string cmdString;
	int targetCmd;
	int doneCmd;
	static std::queue<CHttpSession *> sessionQueue;
};

#endif
