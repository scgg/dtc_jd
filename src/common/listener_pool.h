#include "daemon.h"
#include "sockaddr.h"

class CListener;
class CPollThread;
class CDecoderUnit;
class CTaskRequest;
class CTTCDecoderUnit;
template<typename T> class CTaskDispatcher;

class CListenerPool
{
private:
	CSocketAddress sockaddr[MAXLISTENERS];
	CListener *listener[MAXLISTENERS];
	CPollThread *thread[MAXLISTENERS];
	CTTCDecoderUnit *decoder[MAXLISTENERS];

	int InitDecoder(int n, int idle, CTaskDispatcher<CTaskRequest> *out);
	
public:
	CListenerPool();
	~CListenerPool();
	int Bind(CConfig *, CTaskDispatcher<CTaskRequest> *);

	int Match(const char *, int=0);
	int Match(const char *, const char *);
};

