#ifndef SESSION_H
#define SESSION_H

#include "poller.h"
#include "stream_receiver.h"
#include "sender.h"
#include "http_agent_request_msg.h"
#include "http_agent_response_msg.h"
#include "log.h"
#include "poll_thread.h"

class Session : public CPollerObject, public CTimerObject, public IReceiver{
public:
	Session(CPollThread *thread, int fd) : CPollerObject(thread, fd){
		streamReceiver = new CStreamReceiver(fd, 0, this);
		agentSender = new CAGSender(fd, 1000);
		agentSender->Build();
		timerList = dynamic_cast<CTimerUnit *>(ownerUnit)->GetTimerList(1);
		EnableInput();
		AttachPoller();
	}

	~Session(){
		if (streamReceiver)
			delete streamReceiver;
		if (agentSender)
			delete agentSender;
	}

	virtual void InputNotify(){
		if (streamReceiver->Recv() < 0)
			CloseConnection();
	}

	virtual void TimerNotify(){
		CloseConnection();
	}

	virtual int HandleHeader(void *pPkg, int iBytesRecved, int *piPkgLen){
		CHttpAgentRequestMsg decoder;
		return decoder.HandleHeader(pPkg, iBytesRecved, piPkgLen);
	}

	virtual int HandlePkg(void *pPkg, int iPkgLen);

	void CloseConnection(){
		DetachPoller();
		delete this;
	}
	
private:
	CStreamReceiver *streamReceiver;
	CAGSender *agentSender;
	CTimerList *timerList;
};

#endif
