#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <alloca.h>
#include <stdlib.h>

#include "log.h"
#include "helper_client.h"
#include "helper_group.h"
#include "unix_socket.h"

extern CTableDefinition *gTableDef[];

CHelperClient::CHelperClient(CPollerUnit *o, CHelperGroup *hg, int idx) :
	CPollerObject (o)
{
	packet = NULL;
	task = NULL;

	helperGroup = hg;
	helperIdx = idx;

	supportBatchKey = 0;
	connectErrorCnt = 0;
	ready = 0;
	Ready(); // 开始默认可用
}

CHelperClient::~CHelperClient ()
{    
	if ((0 != task))
	{
		if(stage == HelperRecvVerifyState) {
			DELETE(task);
		} else if(stage != HelperRecvRepState) {
			QueueBackTask();
		} else {
			if(task->ResultCode() >= 0)
				SetError(-EC_UPSTREAM_ERROR, __FUNCTION__,
						"Server Shutdown");
			task->ReplyNotify();
			task = NULL;
		}
	}

	DELETE(packet);
}

int CHelperClient::Ready()
{
	if(ready == 0){
		helperGroup->AddReadyHelper(); 
	}

	ready = 1;
	connectErrorCnt = 0;

	return 0;
}

int CHelperClient::ConnectError()
{
	connectErrorCnt++;
	if(connectErrorCnt > maxTryConnect && ready){
		log_debug("helper-client[%d] try connect %lu times, switch invalid.",
				helperIdx,
				(unsigned long)connectErrorCnt);
		helperGroup->DecReadyHelper(); 
		ready = 0;
	}

	return 0;
}

int CHelperClient::AttachTask(CTaskRequest *p, CPacket *s) 
{
	log_debug("CHelperClient::AttachTask()");

	task = p;
	packet = s;

	int ret = packet->Send(netfd);
	if (ret==SendResultDone) {
		DELETE(packet);
		stopWatch.start();
		task->PrepareDecodeReply();
		receiver.attach(netfd);
		receiver.erase();

		stage = HelperRecvRepState;
		EnableInput();
	} else {
		stage = HelperSendReqState;
		EnableOutput();
	}

	AttachTimer(helperGroup->recvList);
	return DelayApplyEvents();
}

void CHelperClient::CompleteTask(void) {
	DELETE(packet);
	if(task != NULL) {
		task->ReplyNotify();
		task = NULL;
	}
}

void CHelperClient::QueueBackTask(void) {
	DELETE(packet);
	helperGroup->QueueBackTask(task);
	task = NULL;
}

int CHelperClient::Reset ()
{
	if(stage==HelperSendVerifyState || stage==HelperRecvVerifyState)
	{
		DELETE(packet);
		DELETE(task);
	}
	else{
		if(task != NULL && task->ResultCode() >= 0)
		{
			if(stage==HelperRecvRepState)
				SetError (-EC_UPSTREAM_ERROR,
						"CHelperGroup::Reset", "helper recv error");
			else if(stage==HelperSendReqState)
				SetError (-EC_SERVER_ERROR,
						"CHelperGroup::Reset", "helper send error");
		}
		CompleteTask();
	}

	if(stage == HelperIdleState)
		helperGroup->ConnectionReset(this);

	DisableInput();
	DisableOutput();
	CPollerObject::DetachPoller ();
	if(netfd > 0)
		close(netfd);
	netfd = -1;
	stage = HelperDisconnected;
	AttachTimer(helperGroup->retryList);
	return 0;
}

int CHelperClient::ConnectServer(const char *path)
{
	if(path == NULL || path[0] == '\0')
		return -1;

	if(IsUnixSocketPath(path)) {
		if ((netfd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
		{
			log_error("%s", "socket error,%m");
			return -2;
		}

		fcntl(netfd, F_SETFL, O_RDWR|O_NONBLOCK);

		struct sockaddr_un unaddr;
		socklen_t addrlen;
		addrlen = InitUnixSocketAddress(&unaddr, path);
		return connect (netfd, (struct sockaddr *)&unaddr, addrlen);
	}
	else {
		const char *addr = NULL;
		const char *port = NULL;
		const char *begin= strchr(path, ':');
		if(begin)
		{
			char *p = (char *)alloca(begin-path+1);
			memcpy(p, path, begin-path);
			p[begin-path] = '\0';
			addr = p;
		}
		else
		{
			log_error("address error,correct address is addr:port/protocol");
			return -5;
		}

		const char *end= strchr(path, '/');
		if(begin && end)
		{
			char *p = (char *)alloca(end-begin);
			memcpy(p, begin+1, end-begin-1);
			p[end-begin-1] = '\0';
			port = p;
		}
		else
		{
			log_error("protocol error,correct address is addr:port/protocol");
			return -6;
		}

		struct sockaddr_in inaddr;
		bzero(&inaddr, sizeof(struct sockaddr_in));
		inaddr.sin_family = AF_INET;
		inaddr.sin_port   = htons(atoi(port));

		if(strcmp(addr, "*")!=0 &&
				inet_pton(AF_INET, addr, &inaddr.sin_addr) <= 0)
		{
			log_error("invalid address %s:%s", addr, port);
			return -3;
		}

		if(strcasestr(path, "tcp"))
			netfd = socket(AF_INET, SOCK_STREAM, 0);
		else
			netfd = socket(AF_INET, SOCK_DGRAM, 0);

		if(netfd < 0){
			log_error("%s", "socket error,%m");
			return -4;
		}

        //add by frankyang 2009-09-10 with fd is nonblock
		fcntl(netfd, F_SETFL, O_RDWR|O_NONBLOCK);

		return connect(netfd, (const struct sockaddr *)&inaddr, sizeof(inaddr));
	}
	return 0;
}

int CHelperClient::Reconnect(void) 
{ 	
	// increase connect count
	ConnectError();

	if(stage != HelperDisconnected)
		Reset();

	const char *sockpath = helperGroup->SockPath();
	if(ConnectServer(sockpath) == 0)
	{
		log_debug ("Connected to helper[%d]: %s", helperIdx, sockpath);
		
		packet = new CPacket;
		packet->EncodeDetect(gTableDef[0]);

		if(AttachPoller() != 0){
			log_error("helper[%d] attach poller error", helperIdx);
			return -1;
		}
		DisableInput();
		EnableOutput();
		stage = HelperSendVerifyState;
		return SendVerify();
	}

	if(errno != EINPROGRESS) {
		log_error("connect helper-%s error: %m", sockpath);
		close(netfd);
		netfd = -1;
		AttachTimer(helperGroup->retryList);
		//check helpergroup task queue expire.
		helperGroup->CheckQueueExpire();
		return 0;
	}

	log_debug ("Connectting to helper[%d]: %s", helperIdx, sockpath);

	DisableInput();
	EnableOutput();
	AttachTimer(helperGroup->connList);
	stage = HelperConnecting;
	return AttachPoller();
}

int CHelperClient::SendVerify ()
{
	int ret = packet->Send(netfd);
	if (ret==SendResultDone) {
		DELETE(packet);

		task = new CTaskRequest(gTableDef[0]);
		if(task == NULL){
			log_error("%s: %m", "new task & packet error");
			return -1;
		}
		task->PrepareDecodeReply();
		receiver.attach(netfd);
		receiver.erase();

		stage = HelperRecvVerifyState;
		DisableOutput ();        
		EnableInput();
	} else {
		stage = HelperSendVerifyState;
		EnableOutput();
	}

	AttachTimer(helperGroup->recvList);
	return DelayApplyEvents();
}

int CHelperClient::RecvVerify ()
{
	static int logwarn;
	int ret = task->Decode (receiver);

	supportBatchKey = 0;
	switch (ret)
	{
		default:
		case DecodeFatalError:
			log_error ("decode fatal error retcode[%d] msg[%m] from helper", ret);
			goto ERROR_RETURN;

		case DecodeDataError:
			log_error ("decode data error from helper %d", task->ResultCode());
			goto ERROR_RETURN;

		case DecodeWaitData:
		case DecodeIdle:
			AttachTimer(helperGroup->recvList);
			return 0;

		case DecodeDone:
			switch(task->ResultCode()){
				case -EC_EXTRA_SECTION_DATA:
					supportBatchKey = 1;
					break;
				case -EC_BAD_FIELD_NAME: // old version ttc
					supportBatchKey = 0;
					break;
				default:
					log_error("detect helper-%s error: %d, %s", 
							helperGroup->SockPath(),
							task->ResultCode(), 
							task->resultInfo.ErrorMessage());
					goto ERROR_RETURN;
			}
			break;
	}

	if(supportBatchKey){
		log_debug("helper-%s support batch-key", helperGroup->SockPath());
	}
	else{
		if(logwarn++ == 0)
			log_warning("helper-%s unsupported batch-key", helperGroup->SockPath());
		else
			log_debug("helper-%s unsupported batch-key", helperGroup->SockPath());
	}

	DELETE(task);
	Ready();

	EnableInput();
	DisableOutput();
	stage = HelperIdleState;
	helperGroup->RequestCompleted (this);
	DisableTimer();
	return DelayApplyEvents();

ERROR_RETURN:
	Reset();
	AttachTimer(helperGroup->retryList);
	//check helpergroup task queue expire.
	helperGroup->CheckQueueExpire();
	return 0;
}

//client peer
int CHelperClient::RecvResponse ()
{
	int ret = task->Decode (receiver);

	switch (ret)
	{
		default:
		case DecodeFatalError:
			log_notice ("decode fatal error retcode[%d] msg[%m] from helper", ret);
			task->SetError (-EC_UPSTREAM_ERROR, __FUNCTION__, "decode fatal error from helper");
			break;

		case DecodeDataError:
			log_notice ("decode data error from helper %d", task->ResultCode());
			task->SetError (-EC_UPSTREAM_ERROR, __FUNCTION__, "decode data error from helper");
			break;

		case DecodeWaitData:
		case DecodeIdle:
			AttachTimer(helperGroup->recvList);
			return 0;

		case DecodeDone:
			break;
	}

	stopWatch.stop();
	helperGroup->RecordProcessTime(task->RequestCode(), stopWatch);
	CompleteTask();
	helperGroup->RequestCompleted (this);

	// ??
	EnableInput();
	stage = HelperIdleState;
	if(ret!=DecodeDone) return -1;
	return 0;
}

int CHelperClient::SendRequest ()
{
	int ret = packet->Send (netfd);

	log_debug ("[CHelperClient][task=%d]Send Request result=%d, fd=%d", task->Role (), ret, netfd);

	switch (ret)
	{
		case SendResultMoreData:
			break;

		case SendResultDone:
			DELETE(packet);
			stopWatch.start();
			task->PrepareDecodeReply();
			receiver.attach(netfd);
			receiver.erase();

			stage = HelperRecvRepState;
			DisableOutput ();        
			EnableInput();
			break;

		case SendResultError:
		default:
			log_notice ("send result error, ret = %d msg = %m", ret);
			task->SetError (-EC_SERVER_ERROR, "Data source send failed", NULL);	    
			return -1;
	}

	AttachTimer(helperGroup->recvList);
	return 0;
}

void CHelperClient::InputNotify (void)
{
	DisableTimer();

	if(stage==HelperRecvVerifyState) {
		if(RecvVerify () < 0)
			Reconnect();
		return;
	}else if(stage==HelperRecvRepState) {
		if(RecvResponse () < 0)
			Reconnect();
		return;
	} else if(stage == HelperIdleState) {
		/* no data from peer allowed in idle state */
		Reset();
		return;
	}
	DisableInput();
}

void CHelperClient::OutputNotify (void)
{
	DisableTimer();
	if(stage==HelperSendVerifyState) {
		if(SendVerify () < 0) {
			DELETE(packet);
			Reconnect();
		}
		return;
	} else if(stage==HelperSendReqState) {
		if(SendRequest () < 0) {
			QueueBackTask();
			Reconnect();
		}
		return;
	} else if(stage==HelperConnecting) {
		packet = new CPacket;
		packet->EncodeDetect(gTableDef[0]);

		DisableInput();
		EnableOutput();
		stage = HelperSendVerifyState;
		SendVerify();
		return;
	}
	DisableOutput();
}

void CHelperClient::HangupNotify (void)
{
#if 0
	if(stage!=HelperConnecting)
		Reconnect();
	else
#endif
		Reset();
}

void CHelperClient::TimerNotify (void)
{
	switch(stage)
	{
		case HelperRecvRepState:
			stopWatch.stop();
			helperGroup->RecordProcessTime(task->RequestCode(), stopWatch);

			log_error("helper index[%d] execute timeout.", helperIdx);
			SetError (-EC_UPSTREAM_ERROR, "CHelperGroup::Timeout", "helper execute timeout");
			Reconnect();
			break;
		case HelperSendReqState:

			log_error("helper index[%d] send timeout.", helperIdx);
			SetError (-EC_SERVER_ERROR, "CHelperGroup::Timeout", "helper send timeout");
			Reconnect();
			break;
		case HelperDisconnected:
			Reconnect();
			break;
		case HelperConnecting:
			Reset();
			break;
		case HelperSendVerifyState:
		case HelperRecvVerifyState:
			DELETE(packet);
			DELETE(task);
			Reconnect();
			break;
		default:
			break;
	}
}
