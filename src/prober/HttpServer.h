#include "poller.h"
#include "sockaddr.h"
#include "poll_thread_group.h"
#include "stream_receiver.h"
#include "sender.h"
#include "http_agent_request_msg.h"
#include "http_agent_response_msg.h"
#include "log.h"

class CHttpSessionLogic
{
public:
	virtual bool ProcessBody(std::string reqBody, std::string &respBody) = 0;
};

class CHttpAcceptor : public CPollerObject
{
public:
	CHttpAcceptor(CPollThreadGroup *o, CHttpSessionLogic *s):CPollerObject(o->GetPollThread()){
		pollThreadGroup = o;
		sessionLogic = s;
	};

	int ListenOn(const std::string &addr);
	virtual void InputNotify();
private:
	CSocketAddress bindAddr;
	CPollThreadGroup *pollThreadGroup;
	CHttpSessionLogic *sessionLogic;
};

class CHttpServer
{
public:
	CHttpServer(CPollThreadGroup *o, CHttpSessionLogic *s){
		pollThreadGroup = o;
		httpAcceptor = new CHttpAcceptor(pollThreadGroup, s);
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
	CPollThreadGroup *pollThreadGroup;
};

class CHttpSession : public CPollerObject, public CTimerObject, public IReceiver
{
public:
	CHttpSession(CPollThreadGroup *o, int fd, CHttpSessionLogic *s):CPollerObject(o->GetPollThread(), fd){
		EnableInput();
		AttachPoller();
		streamReceiver = new CStreamReceiver(fd, 0, this);
		agentSender = new CAGSender(fd, 1000);
		sessionLogic = s;
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
		log_info("handle header buffer:%s bufferlen:%d", (char*)pPkg, iBytesRecved);
		CHttpAgentRequestMsg decoder;
		return decoder.HandleHeader(pPkg, iBytesRecved, piPkgLen);
	}

	virtual int HandlePkg(void *pPkg, int iPkgLen);

	void CloseConnection()
	{
		DetachPoller();
		//close(netfd);
		delete this;
	}
private:
	CPollThreadGroup   *ownerThread;
	CStreamReceiver    *streamReceiver;
	CAGSender          *agentSender;
	CTimerList         *timerList;
	CHttpSessionLogic  *sessionLogic;
};
