#ifndef __LISTENER_H__
#define __LISTENER_H__
#include "poller.h"
#include "sockaddr.h"

int UnixSockBind (const char *path, int backlog=0);
int UdpSockBind (const char *addr, uint16_t port, int rbufsz, int wbufsz);
int SockBind (const char *addr, uint16_t port, int backlog=0);

class CDecoderUnit;
class CListener : public CPollerObject {
public:
	CListener (const CSocketAddress *);
	~CListener();

	int Bind(int blog, int rbufsz, int wbufsz);
	virtual void InputNotify(void);
	virtual int Attach (CDecoderUnit *, int blog=0, int rbufsz=0, int wbufsz=0);
	void SetRequestWindow(int w) { window = w; };
	int FD(void) const { return netfd; }

private:
	CDecoderUnit *outPeer;
	const CSocketAddress *sockaddr;
	int bind;
	int window;
}; 
#endif
