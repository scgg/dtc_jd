/*
 * =====================================================================================
 *
 *       Filename:  agent_listener.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/01/2010 12:36:41 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __AGENT_LISTENER_H__
#define __AGENT_LISTENER_H__

#include "poller.h"
#include "sockaddr.h"

class CPollThread;
class CAgentClientUnit;
class CAgentListener : public CPollerObject
{
    private:
	CPollThread * ownerThread;
	CAgentClientUnit * out;
	const CSocketAddress addr;
	
    public:
	CAgentListener(CPollThread * t, CAgentClientUnit * o, CSocketAddress & a);
	virtual ~CAgentListener();

	int Bind(int blog);
	int AttachThread();
    private:
	virtual void InputNotify();
};

#endif
