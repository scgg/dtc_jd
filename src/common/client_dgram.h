#ifndef __CLIENT__DGRAM_H__
#define __CLIENT__DGRAM_H__

#include <sys/socket.h>
#include "poller.h"
#include "task_request.h"
#include "packet.h"
#include "timerlist.h"

class CTTCDecoderUnit;
class CClientDgram;

struct CDgramInfo
{
	CClientDgram *cli;
	socklen_t len;
	char addr[0];
};

class CClientDgram : public CPollerObject {
public:
	CTTCDecoderUnit *owner;

	CClientDgram (CTTCDecoderUnit *, int fd);
	virtual ~CClientDgram ();

	virtual int Attach (void);
	int SendResult(CTaskRequest *, void *, int);

private:
	virtual void InputNotify (void);

protected:
	// recv on empty&no packets
	int RecvRequest (int noempty);
private:
	int hastrunc;
	int mru;
	int mtu;
	int alen; // address length
	CDgramInfo *abuf; // current packet address

private:
	int AllocateDgramInfo(void);
	int InitSocketInfo(void);
};

#endif

