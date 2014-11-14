#ifndef __TTC_CONTAINER_HEADER__
#define __TTC_CONTAINER_HEADER__

class CTableDefinition;
class CTaskRequest;
class NCRequest;
class NCResultInternal;
union CValue;

#if GCC_MAJOR >= 4
#pragma GCC visibility push(default)
#endif
class IInternalService {
	public:
		virtual ~IInternalService(void){}
		virtual const char *QueryVersionString(void) = 0;
		virtual const char *QueryServiceType(void) = 0;
		virtual const char *QueryInstanceName(void) = 0;
};

class ITTCTaskExecutor {
	public:
		virtual ~ITTCTaskExecutor(void) {}
		virtual NCResultInternal * TaskExecute(NCRequest &, const CValue *) = 0;
};

class ITTCService: public IInternalService {
	public:
		virtual ~ITTCService(void) {}
		virtual CTableDefinition *QueryTableDefinition(void) = 0;
		virtual CTableDefinition *QueryAdminTableDefinition(void) = 0;
		virtual ITTCTaskExecutor *QueryTaskExecutor(void) = 0;
		virtual int MatchListeningPorts(const char *, const char * = 0) = 0;
};
#if GCC_MAJOR >= 4
#pragma GCC visibility pop
#endif

IInternalService *QueryInternalService(const char *name, const char *instance);

#endif
