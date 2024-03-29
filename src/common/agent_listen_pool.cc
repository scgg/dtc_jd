#include "agent_listen_pool.h"
#include "agent_listener.h"
#include "agent_client_unit.h"
#include "poll_thread.h"
#include "config.h"
#include "task_request.h"
#include "log.h"
#include "StatTTC.h"

CAgentListenPool::CAgentListenPool()
{
    memset(thread, 0, sizeof(CPollThread *) * MAX_AGENT_LISTENER);
    memset(out, 0, sizeof(CAgentClientUnit *) * MAX_AGENT_LISTENER);
    memset(listener, 0, sizeof(CAgentListener *) * MAX_AGENT_LISTENER);

    CStatItemU32 statAgentCurConnCount = statmgr.GetItemU32(AGENT_CONN_COUNT);
    statAgentCurConnCount = 0;
}

CAgentListenPool::~CAgentListenPool()
{
    for(int i = 0; i < MAX_AGENT_LISTENER; i++)
    {
	if(thread[i])
	    thread[i]->interrupt();
	delete listener[i];
	delete out[i];
    }
}

/* part of framework construction */
int CAgentListenPool::Bind(CConfig * gc, CTaskDispatcher<CTaskRequest> * agentprocess)
{
    char bindstr[64];
    const char * bindaddr;
    const char * errmsg = NULL;
    char threadname[64];
    int checktime;
    int blog;

    checktime = gc->GetIntVal("cache", "AgentRcvBufCheck", 5);
    blog = gc->GetIntVal("cache", "AgentListenBlog", 256);

    for(int i = 0; i < MAX_AGENT_LISTENER; i++)
    {
	if(i == 0)
	    snprintf(bindstr, sizeof(bindstr), "BindAddr");
	else
	    snprintf(bindstr, sizeof(bindstr), "BindAddr%d", i);

	bindaddr = gc->GetStrVal("cache", bindstr);
	if(NULL == bindaddr)
	    continue;

	if((errmsg = addr[i].SetAddress(bindaddr, (const char *)NULL)))
	    continue;

	snprintf(threadname, sizeof(threadname), "inc%d", i);
	thread[i] = new CPollThread(threadname);
	if(NULL == thread[i])
	{
	    log_error("no mem to new agent inc thread %d", i);
	    return -1;
	}
	if(thread[i]->InitializeThread() < 0)
	{
	    log_error("agent inc thread %d init error", i);
	    return -1;
	}

	out[i] = new CAgentClientUnit(thread[i], checktime);
	if(NULL == out[i])
	{
	    log_error("no mem to new agent client unit %d", i);
	    return -1;
	}
	out[i]->BindDispatcher(agentprocess);

	listener[i] = new CAgentListener(thread[i], out[i], addr[i]);
	if(NULL == listener[i])
	{
	    log_error("no mem to new agent listener %d", i);
	    return -1;
	}
	if(listener[i]->Bind(blog) < 0)
	{
	    log_error("agent listener %d bind error", i);
	    return -1;
	}

	if(listener[i]->AttachThread() < 0)
	    return -1;
    }

    return 0;
}

int CAgentListenPool::Run()
{
    for(int i = 0; i < MAX_AGENT_LISTENER; i++)
    {
	if(thread[i])
	    thread[i]->RunningThread();
    }

    return 0;
}

int CAgentListenPool::Match(const char *host, const char *port)
{
    for(int i=0; i<MAX_AGENT_LISTENER; i++) 
    {
        if(listener[i]==NULL)
            continue;
	if(addr[i].Match(host, port))
	    return 1;
    }
    return 0;
}
