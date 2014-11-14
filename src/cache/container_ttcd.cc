#include <stdio.h>
#include <map>

#include "compiler.h"
#include "container.h"
#include "version.h"
#include "tabledef.h"
#include "cache_error.h"
#include "listener_pool.h"
#include "request_threading.h"
#include "task_multiplexer.h"
#include "../api/c_api/ttcint.h"
#include "agent_listen_pool.h"

class CTTCTaskExecutor: public ITTCTaskExecutor, public CThreadingOutputDispatcher<CTaskRequest>
{
	public:
		virtual NCResultInternal *TaskExecute(NCRequest &rq, const CValue *kptr);
};

NCResultInternal *CTTCTaskExecutor::TaskExecute(NCRequest &rq, const CValue *kptr) {
	NCResultInternal *res = new NCResultInternal(rq.tdef);
	if(res->Copy(rq, kptr) < 0)
		return res;
	res->SetOwnerInfo(this, 0, NULL);
	switch(CThreadingOutputDispatcher<CTaskRequest>::Execute((CTaskRequest *)res))
	{
		case 0: // OK
			res->ProcessInternalResult(res->Timestamp());
			break;
		case -1: // no side effect
			res->SetError(-EC_REQUEST_ABORTED, "API::sending", "Server Shutdown");
			break;
		case -2:
		default: // result unknown, leak res by purpose
			//new NCResult(-EC_REQUEST_ABORTED, "API::recving", "Server Shutdown");
            log_error("(-EC_REQUEST_ABORTED, API::sending, Server Shutdown");
			break;
	}
	return res;
}

class CTTCInstance: public ITTCService {
	public:
		CTableDefinition *tableDef;
		CAgentListenPool *ports;
		CTTCTaskExecutor *executor;
		int mypid;

	public:
		CTTCInstance();
		virtual ~CTTCInstance();

		virtual const char *QueryVersionString(void);
		virtual const char *QueryServiceType(void);
		virtual const char *QueryInstanceName(void);

		virtual CTableDefinition *QueryTableDefinition(void);
		virtual CTableDefinition *QueryAdminTableDefinition(void);
		virtual ITTCTaskExecutor *QueryTaskExecutor(void);
		virtual int MatchListeningPorts(const char *, const char * = NULL);

		int IsOK(void) const
		{
			return	this != NULL &&
				tableDef != NULL &&
				ports != NULL &&
				executor != NULL &&
				getpid() == mypid;

		}
};

extern CListenerPool *listener;
CTTCInstance::CTTCInstance(void)
{
	tableDef = NULL;
	ports = NULL;
	executor = NULL;
	mypid = getpid();
}

CTTCInstance::~CTTCInstance(void)
{
}

const char *CTTCInstance::QueryVersionString(void)
{
	return version_detail;
}

const char *CTTCInstance::QueryServiceType(void)
{
	return "ttcd";
}

const char *CTTCInstance::QueryInstanceName(void)
{
	return tableDef->TableName();
}

CTableDefinition *CTTCInstance::QueryTableDefinition(void)
{
	return tableDef;
}

CTableDefinition *CTTCInstance::QueryAdminTableDefinition(void)
{
	return gTableDef[1];
}

ITTCTaskExecutor *CTTCInstance::QueryTaskExecutor(void)
{
	return executor;
}


int CTTCInstance::MatchListeningPorts(const char *host, const char *port)
{
	return ports->Match(host, port);
}


struct nocase
{
	bool operator()(const char * const & a, const char * const & b) const
	{ return strcasecmp(a, b) < 0; }
};
typedef std::map<const char *, CTTCInstance, nocase> instmap_t;
static instmap_t instMap;


extern "C"
__EXPORT
IInternalService *_QueryInternalService(const char *name, const char *instance)
{
	instmap_t::iterator i;

	if(!name || !instance)
		return NULL;

	if(strcasecmp(name, "ttcd") != 0)
		return NULL;

	/* not found */
	i = instMap.find(instance);
	if(i == instMap.end())
		return NULL;

	CTTCInstance &inst = i->second;
	if(inst.IsOK() == 0)
		return NULL;

	return &inst;
}

void InitTaskExecutor(CTableDefinition *tdef, CAgentListenPool *listener, CTaskDispatcher<CTaskRequest> *output) 
{
	if(NCResultInternal::VerifyClass()==0) {
		log_error("Inconsistent class NCResultInternal detected, internal API disabled");
		return;
	}
	const char *tablename = tdef->TableName();
	CTTCInstance &inst = instMap[tablename];
	inst.tableDef = tdef;
	inst.ports = listener;
	CTTCTaskExecutor *executor = new CTTCTaskExecutor();
	CTaskMultiplexer *batcher = new CTaskMultiplexer(output->OwnerThread());
	batcher->BindDispatcher(output);
	executor->BindDispatcher(batcher);
	inst.executor = executor;
	log_info("Internal Task Executor initialized");
}

void StopTaskExecutor(void) {
	instmap_t::iterator i;
	for(i = instMap.begin(); i != instMap.end(); i++) {
		if(i->second.executor)
			i->second.executor->Stop();
	}
}
