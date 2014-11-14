#ifndef __CLIENT_SYNC_H__
#define __CLIENT_SYNC_H__

#include "poller.h"
#include "timerlist.h"
#include "receiver.h"

class CTTCDecoderUnit;
class CTaskRequest; 
class CPacket;
class CClientResourceSlot;

class CClientSync : public CPollerObject, private CTimerObject {
public:
    CTTCDecoderUnit *owner;
    void *addr;
    int addrLen;
    /* newman: pool */
    unsigned int resourceId;
    CClientResourceSlot * resource;
    uint32_t resourceSeq;
    enum RscStatus
    {
	RscClean,
	RscDirty,
    };
    RscStatus rscStatus;

    CClientSync (CTTCDecoderUnit *, int fd, void *, int);
    virtual ~CClientSync ();

    virtual int Attach (void);
    int SendResult(void);

    virtual void InputNotify (void);
private:
    virtual void OutputNotify (void);

    /* newman: pool */
    int GetResource();
    void FreeResource();
    void CleanResource();
    
protected:	
    enum CClientState 
    {
	IdleState,
	RecvReqState, //wait for recv request, server side
	SendRepState, //wait for send response, server side
	ProcReqState, // IN processing
    };

    CSimpleReceiver receiver;
    CClientState stage;
    CTaskRequest *task;
    CPacket *reply;

    int RecvRequest (void);
    int Response (void);
    void AdjustEvents(void);
};

#endif

