/*
 * =====================================================================================
 *
 *       Filename:  agent_listen_poll.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/01/2010 12:58:29 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __AGENT_LISTEN_POOL_H__
#define __AGENT_LISTEN_POOL_H__

#include "request_base.h"
#include "sockaddr.h"

#define MAX_AGENT_LISTENER 10

class CAgentClientUnit;
class CAgentListener;
class CPollThread;
class CConfig;
class CTaskRequest;
class CAgentListenPool
{
    private:
	CSocketAddress addr[MAX_AGENT_LISTENER];
	CPollThread * thread[MAX_AGENT_LISTENER];
	CAgentClientUnit * out[MAX_AGENT_LISTENER];
	CAgentListener * listener[MAX_AGENT_LISTENER];

    public:
	CAgentListenPool();
	~CAgentListenPool();

	int Bind(CConfig *gc, CTaskDispatcher<CTaskRequest> *out);
	int Run();
	int Match(const char *host, const char *port);
};

#endif
