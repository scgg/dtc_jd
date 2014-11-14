#ifndef __HELPER_CLIENT_H__
#define __HELPER_CLIENT_H__

#include "poller.h"
#include "packet.h"
#include "timerlist.h"
#include "task_request.h"
#include "stopwatch.h"

enum CHelperState 
{
	HelperDisconnected = 0,
	HelperConnecting,
	HelperIdleState,
	HelperRecvRepState, //wait for recv response, client side
	HelperSendReqState, //wait for send request, client side
	HelperSendVerifyState,
	HelperRecvVerifyState,
};

class CHelperGroup;

/*
  State Machine: CHelperClient object is static, beyond reconnect
 	Disconnected
		wait retryTimeout --> trying reconnect
 	Reconnect
		If connected --> IdleState
        	If inprogress --> ConnectingState
	ConnectingState
		If HangupNotify --> Disconnected
		If OutputNotify --> SendVerifyState
		If TimerNotify:connTimeout --> Disconnected
	SendVerifyState
		If HangupNotify --> Disconnected
		If OutputNotify --> Trying Sending
		If TimerNotify:connTimeout --> Disconnected
	RecvVerifyStat
		If HangupNotify --> CompleteTask(error) -->Reconnect
		If InputNotify --> DecodeDone --> IdleState
	IdleState
		If HangupNotify --> Reconnect
		If AttachTask --> Trying Sending
	Trying Sending
		If Sent --> RecvRepState
		If MoreData --> SendRepState
		If SentError --> PushBackTask --> Reconnect
	SendRepState
		If HangupNotify --> PushBackTask -->Reconnect
		If OutputNotify --> Trying Sending
	RecvRepState
		If HangupNotify --> CompleteTask(error) -->Reconnect
		If InputNotify --> Decode Reply
	DecodeReply
		If DecodeDone --> IdleState
		If MoreData --> RecvRepState
		If FatalError --> CompleteTask(error) --> Reconnect
		If DataError --> CompleteTask(error) --> Reconnect
	
 */
class CHelperClient :
	public CPollerObject,
	private CTimerObject
{
public:
	friend class CHelperGroup;

	CHelperClient (CPollerUnit *, CHelperGroup *hg, int id);
	virtual ~CHelperClient ();

	int AttachTask (CTaskRequest *, CPacket *);
	
	int SupportBatchKey(void) const { return supportBatchKey; }
	
private:
	int Reset ();
	int Reconnect ();

	int SendVerify();
	int RecvVerify();
	
	int Ready();
	int ConnectError();
	
	void CompleteTask(void);
	void QueueBackTask(void);
	void SetError(int err, const char *msg, const char *msg1)
	{
		task->SetError(err, msg, msg1);
	}
	void SetErrorDup(int err, const char *msg, const char *msg1)
	{
		task->SetErrorDup(err, msg, msg1);
	}

public:
	const char *StateString(void) {
		return this==NULL ? "NULL" :
			((const char *[]) {
				"DISC", "CONN", "IDLE", "RECV", "SEND", "SND_VER", "RECV_VER", "BAD"
			}) [ stage ];
	}

private:
	virtual void InputNotify (void);
	virtual void OutputNotify (void);
	virtual void HangupNotify (void);
	virtual void TimerNotify (void);

private:	
	int RecvResponse ();
	int SendRequest ();
	int ConnectServer(const char *path);

	CSimpleReceiver receiver;
	CTaskRequest *task;
	CPacket *packet;
	
	CTaskRequest *verify_task;
	CPacket *verify_packet;
	
	CHelperGroup *helperGroup;
	int helperIdx;

	CHelperState stage;
	
	int supportBatchKey;
	static const unsigned int maxTryConnect = 10;
	uint64_t connectErrorCnt;
	int ready;
	stopwatch_usec_t stopWatch;
};
#endif

