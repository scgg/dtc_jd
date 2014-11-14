/*
 * =====================================================================================
 *
 *       Filename:  agent_agent_unit.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/30/2010 01:06:12 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __AGENT_CLIENT_UNIT_H__
#define __AGENT_CLIENT_UNIT_H__

#include "request_base_all.h"
#include "StatTTC.h"

class CTimerList;
class CPollThread;
class CTaskRequest;
class CAgentClientUnit
{
    public:
	CAgentClientUnit(CPollThread * t, int c);
	virtual ~CAgentClientUnit();

	inline CTimerList * TimerList() { return tlist; }
	void BindDispatcher(CTaskDispatcher<CTaskRequest> * proc);
	inline void TaskNotify(CTaskRequest * req) { output.TaskNotify(req); }
	void RecordRequestTime(int hit, int type, unsigned int usec);
	void RecordRequestTime(CTaskRequest *req);

    private:
	CPollThread * ownerThread;
	CRequestOutput<CTaskRequest> output;
	int check;
	CTimerList * tlist;

        CStatSample statRequestTime[8];
};

#endif
