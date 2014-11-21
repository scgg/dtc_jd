#ifndef __PLUGIN_LISTENER_POOL_H__
#define __PLUGIN_LISTENER_POOL_H__

#include "daemon.h"
#include "plugin_unit.h"
#include "sockaddr.h"

class CListener;
class CPollThread;

class CPluginAgentListenerPool
{
public:
	CPluginAgentListenerPool();
	~CPluginAgentListenerPool();
	int Bind();

	int Match(const char *, int=0);
	int Match(const char *, const char *);

private:
	CSocketAddress          _sockaddr[MAXLISTENERS];
	CListener*              _listener[MAXLISTENERS];
	CPollThread*            _thread[MAXLISTENERS];
	CPluginDecoderUnit*     _decoder[MAXLISTENERS];
	int                     _udpfd[MAXLISTENERS];

	int InitDecoder(int n, int idle);
};

#endif
