#ifndef __CLIENT__ASYNC_H__
#define __CLIENT__ASYNC_H__

#include "poller.h"
#include "task_request.h"
#include "packet.h"
#include "timerlist.h"
#include "list.h"

class CTTCDecoderUnit;
class CClientAsync;

class CAsyncInfo: private CListObject<CAsyncInfo>
{
public:
	CAsyncInfo(CClientAsync *c, CTaskRequest *r);
	~CAsyncInfo();
	void ListMoveTail(CListObject<CAsyncInfo> *a) {
		CListObject<CAsyncInfo>::ListMoveTail(a);
	}

public:
	CClientAsync *cli;
	CTaskRequest *req;
	CPacket *pkt;
};

class CClientAsync : public CPollerObject {
public:
	friend class CAsyncInfo;
	CTTCDecoderUnit *owner;

	CClientAsync (CTTCDecoderUnit *, int fd, int depth, void *peer, int ps);
	virtual ~CClientAsync ();

	virtual int Attach (void);
	int QueueResult(CAsyncInfo *);
	int QueueError(void);
	int FlushResult(void);
	int AdjustEvents(void);

	virtual void InputNotify (void);
private:
	virtual void OutputNotify (void);

	int RecvRequest (void);
	int Response (void);
	int ResponseOne (void);

protected:	
	CSimpleReceiver receiver;
	CTaskRequest *curReq;	// decoding
	CPacket *curRes;	// sending

	CListObject<CAsyncInfo> waitList;
	CListObject<CAsyncInfo> doneList;

	void *addr;
	int   addrLen;
	int maxReq;
	int numReq;
};

#endif

