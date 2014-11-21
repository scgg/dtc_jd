#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <dlfcn.h>

#include <ttc_global.h>
#include <version.h>
#include <proctitle.h>
#include <log.h>
#include <config.h>
#include <dbconfig.h>
#include "CommProcess.h"
#include <daemon.h>
#include <sockaddr.h>
#include <listener.h>
#include <unix_socket.h>
#include <watchdog_listener.h>
#include <task_base.h>

const char service_file[] = "./helper-service.so";
const char create_handle_name[] = "create_process";
const char progname[] = "custom-helper";
const char usage_argv[] = "machId addr [port]";
char cacheFile[256] = CACHE_CONF_NAME;
char tableFile[256] = TABLE_CONF_NAME;

static CreateHandle CreateHelper = NULL;
static CCommHelper *helperProc;
static unsigned int procTimeout;

/*
class CHelperMain{
public:
	static void Attach(CCommHelper* h, CTask* task){ h->Attach((void*)task); }
	static void InitTitle(CCommHelper* h, int group, int role){ h->InitTitle(group, role); }
	static void SetTitle(CCommHelper* h, const char *status){ h->SetTitle(status); }
	static const char *Name(const CCommHelper* h){ return h->Name(); }
};
*/
class CHelperMain{
public:
	CHelperMain(CCommHelper* helper):h(helper){};
	
	void Attach(CTask* task){ h->Attach((void*)task); }
	void InitTitle(int group, int role){ h->InitTitle(group, role); }
	void SetTitle(const char *status){ h->SetTitle(status); }
	const char *Name(){ return h->Name(); }
	int Preinit(int gid, int r) {
		if(dbConfig->machineCnt <= gid){
			log_error("parse config error, machineCnt[%d] <= GroupID[%d]", dbConfig->machineCnt, gid);
			return(-1);
		}
		h->GroupID = gid;
		h->Role = r;
		h->_dbconfig = dbConfig;
		h->_tdef = gTableDef[0];
		h->_config = gConfig;
		h->_server_string = dbConfig->mach[gid].role[r].addr;
		h->logapi.InitTarget(h->_server_string);
		return 0;
	}
private:
	CCommHelper* h;
};


static int LoadService(const char* dll_file)
{
	void* dll;
	
	dll = dlopen(dll_file, RTLD_NOW|RTLD_GLOBAL);
	if(dll == (void*)NULL){
		log_crit("dlopen(%s) error: %s", dll_file, dlerror());
		return -1;
	}
	
	CreateHelper = (CreateHandle)dlsym(dll, create_handle_name);
	if(CreateHelper == NULL){
		log_crit("function[%s] not found", create_handle_name);
		return -2;
	}
	
	return 0;
}

static int SyncDecode(CTask *task, int netfd, CCommHelper *helperProc) {
	CSimpleReceiver receiver(netfd);
	int code;
	do {
		code = task->Decode(receiver);
		if(code==DecodeFatalError) {
		    if(errno != 0)
			log_notice ("decode fatal error, fd=%d, %m", netfd);
		    return -1;
		}
		if(code==DecodeDataError) {
		    if(task->ResultCode() == 0 || task->ResultCode() == -EC_EXTRA_SECTION_DATA) // -EC_EXTRA_SECTION_DATA   verify package 
			    return 0;
		    log_notice ("decode error, fd=%d, %d", netfd, task->ResultCode());
		    return -1;
		}
		CHelperMain(helperProc).SetTitle("Receiving...");
	} while (!stop && code != DecodeDone);

	if(task->ResultCode() < 0) {
		log_notice ("register result, fd=%d, %d", netfd, task->ResultCode());
		return -1;
	}
	return 0;
}

static int SyncSend(CPacket *reply, int netfd) {
	int code;
	do {
		code = reply->Send(netfd);
		if(code==SendResultError)
		{
		    log_notice ("send error, fd=%d, %m", netfd);
		    return -1;
		}
	} while (!stop && code != SendResultDone);

	return 0;
}


static void alarm_handler(int signo) {
	if(background==0 && getppid()==1)
		exit(0);
	alarm(10);
}

static int AcceptConnection(int fd) {
	CHelperMain(helperProc).SetTitle("listener");
	signal(SIGALRM, alarm_handler);
	while(!stop) {
		alarm(10);
		int newfd;
		if((newfd = accept(fd, NULL, 0)) >= 0) {
			alarm(0);
			return newfd;
		}
		if(newfd < 0 && errno == EINVAL){
			if(getppid() == (pid_t)1){ // 父进程已经退出
				log_error ("ttc father process not exist. helper[%d] exit now.", getpid());
				exit(0);
			}
			usleep(10000);
		}
	}
	exit(0);
}

static void proc_timeout_handler(int signo) {
	log_error ("comm-helper process timeout(more than %u seconds), helper[pid: %d] exit now.", procTimeout, getpid());
	exit(-1);
}

struct THelperProcParameter {
	int netfd;
	int gid;
	int role;
};

static int HelperProcRun (struct THelperProcParameter *args)
{
	// close listen fd
	close(0);
	open("/dev/null", O_RDONLY);

	CHelperMain(helperProc).SetTitle("Initializing...");

	if(procTimeout > 0)
		signal(SIGALRM, proc_timeout_handler);
	
	alarm(procTimeout);
	if (helperProc->Init() != 0)
	{
		log_error ("%s", "helper process init failed");
		exit(-1);
	} 

	alarm(0);
	
	unsigned int timeout;
	
	while(!stop) {
		CHelperMain(helperProc).SetTitle("Waiting...");
		CTask *task = new CTask(gTableDef[0]);
		if(SyncDecode(task, args->netfd, helperProc) < 0) {
			delete task;
			break;
		}
		
		if(task->ResultCode() == 0){
			switch(task->RequestCode()){
				case DRequest::Insert:
				case DRequest::Update:
				case DRequest::Delete:
				case DRequest::Replace:
					timeout = 2*procTimeout;
				default:
					timeout = procTimeout;
			}
			CHelperMain(helperProc).Attach(task);
			alarm(timeout);
			helperProc->Execute();
			alarm(0);
		}
		
		CHelperMain(helperProc).SetTitle("Sending...");
		CPacket *reply = new CPacket;
		reply->EncodeResult(task);
		
		delete task;
		if(SyncSend(reply, args->netfd) <0) {
			delete reply;
			break;
		}
		delete reply;
	}
	close(args->netfd);
	CHelperMain(helperProc).SetTitle("Exiting...");

	delete helperProc;
	DaemonCleanup();
#if MEMCHECK
	log_info("%s v%s: stopped", progname, version);
	dump_non_delete();
	log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());
#endif
	exit(0);
	return 0;
}

int main(int argc, char **argv)
{
	init_proc_title(argc, argv);
	if(TTC_DaemonInit (argc, argv) < 0)
		return -1;

	argc -= optind;
	argv += optind;

	struct THelperProcParameter helperArgs = { 0, 0, 0 };
	char *addr = NULL;

	if(argc > 0)
	{
		char *p;
		helperArgs.gid = strtol(argv[0], &p, 0);
		if(*p=='\0' || *p==MACHINEROLESTRING[0])
			helperArgs.role = 0;
		else if(*p==MACHINEROLESTRING[1])
			helperArgs.role = 1;
		else {
			log_error("Bad machine id: %s", argv[0]);
			return -1;
		}
	}

	if(argc != 2 && argc != 3)
	{
		ShowUsage();
		return -1;
	}

	int backlog = gConfig->GetIntVal("cache", "MaxListenCount", 256);
	int helperTimeout = gConfig->GetIntVal("cache", "HelperTimeout", 30);
	if(helperTimeout > 1)
		procTimeout = helperTimeout -1;
	else
		procTimeout = 0;
	addr = argv[1];

	// load dll file
	const char *file = getenv("HELPER_SERVICE_FILE");
	if(file==NULL || file[0]==0)
		file = service_file;
	if(LoadService(file) != 0)
		return -1;
	
	helperProc = CreateHelper();
	if(helperProc == NULL){
		log_crit("create helper error");
		return -1;
	}
	if( CHelperMain(helperProc).Preinit(helperArgs.gid, helperArgs.role) < 0)
	{
		log_error ("%s", "helper prepare init failed");
		exit(-1);
	}
	if(helperProc->GobalInit() != 0){
		log_crit("helper gobal-init error");
		return -1;
	}
	
	int fd = -1;
	if(!strcmp(addr, "-"))
		fd = 0;
	else {
		if(strcasecmp(gConfig->GetStrVal("cache", "CacheShmKey")?:"", "none") != 0)
		{
			log_warning("standalone %s need CacheShmKey set to NONE", progname);
			return -1;
		}

		CSocketAddress sockaddr;
		const char *err = sockaddr.SetAddress(addr, argc==2 ? NULL : argv[2]);
		if(err) {
			log_warning("host %s port %s: %s", addr, argc==2 ? "NULL" : argv[2], err);
			return -1;
		}
		if(sockaddr.SocketType() != SOCK_STREAM) {
			log_warning("standalone %s don't support UDP protocol", progname);
			return -1;
		}
		fd = SockBind(&sockaddr, backlog);
		if(fd < 0)
			return -1;
	}

	DaemonStart();
	
#if HAS_LOGAPI
	helperProc->logapi.Init(
			gConfig->GetIntVal("LogApi", "MessageId", 0),
			gConfig->GetIntVal("LogApi", "CallerId", 0),
			gConfig->GetIntVal("LogApi", "TargetId", 0),
			gConfig->GetIntVal("LogApi", "InterfaceId", 0)
	);
#endif

	CHelperMain(helperProc).InitTitle(helperArgs.gid, helperArgs.role);
	if(procTimeout > 1)
		helperProc->SetProcTimeout(procTimeout-1);
	while(!stop) {
		helperArgs.netfd = AcceptConnection(fd);

		WatchDogFork(CHelperMain(helperProc).Name(), (int(*)(void*))HelperProcRun, (void *)&helperArgs);

		close(helperArgs.netfd);
	}

	if(fd > 0 && addr && addr[0]=='/')
		unlink(addr);
	return 0;
}

