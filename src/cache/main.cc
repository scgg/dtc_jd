/*                               -*- Mode: C -*- 
 * Filename: 
 * 
 * Keywords: 
 * 
 *       This is unpublished proprietary
 *       source code of Tencent Ltd.
 *       The copyright notice above does not
 *       evidence any actual or intended
 *       publication of such source code.
 *
 *       NOTICE: UNAUTHORIZED DISTRIBUTION,ADAPTATION OR 
 *       USE MAY BE SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *  $Id: main.cc 8187 2011-01-14 07:20:20Z newmanwang $
 */

/* Change log:
 * 	1. 简单做了ttc、bitmap合并，增加了mdetect来根据
	   配置文件侦测svr类型。
 * 
 * 
 */

/* Code: */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <version.h>
#include <tabledef.h>
#include <config.h>
#include <poll_thread.h>
#include <listener_pool.h>
#include <barrier_unit.h>
#include <client_unit.h>
#include <helper_collect.h>
#include <helper_group.h>
#include <cache_process.h>
#include <cache_bypass.h>
#include <watchdog.h>
#include <dbconfig.h>
#include <log.h>
#include <daemon.h>
#include <pipetask.h>
#include <memcheck.h>
#include "unix_socket.h"
#include "StatTTC.h"
#include "svr_control.h"
#include "task_multiplexer.h"
#include "black_hole.h"
#include "container.h"
#include "proctitle.h"
#include "trunner.h"
#include "plugin_mgr.h"
#include "ttc_global.h"
#include "dynamic_helper_collection.h"
#include "key_route.h"
#include "agent_listen_pool.h"
#include "agent_process.h"
#include "version.h"

#include "RelativeHourCalculator.h"

using namespace ClusterConfig;
const char progname[] = "dtcd";
const char usage_argv[] = "";
char cacheFile[256] = CACHE_CONF_NAME;
char tableFile[256] = TABLE_CONF_NAME;
int gMaxConnCnt;

static CPollThread     *cacheThread;
static CCacheProcess   *cachePool;
static int      enablePlugin;
static int      initPlugin;
static int		disableCache;
static int		cacheKey;
static int		disableDataSource;
static CPollThread     *dsThread;
static CGroupCollect   *helperUnit;
static CBarrierUnit 	*barCache;
static CBarrierUnit 	*barHelper;
static CCacheBypass     *bypassUnit;
static CListenerPool	*listener;
static CServerControl	*serverControl;
static CPluginManager   *plugin_mgr;
CKeyRoute*  keyRoute;
static int asyncUpdate;
int targetNewHash;
int hashChanging;
static CAgentListenPool *agentListener;
static CAgentProcess   *agentProcess;


//remote dispatcher,用来迁移数据给远端ttc
static CPollThread * remoteThread;
static CDynamicHelperCollection *remoteClient;

extern CConfig          *gConfig;    
extern CTableDefinition *gTableDef[];

static int plugin_start (void)
{
    initPlugin = 0;

    plugin_mgr = CPluginManager::Instance();
    if (NULL == plugin_mgr)
    {
        log_error ("create CPluginManager instance failed.");
        return -1;
    }

    if (plugin_mgr->open(gConfig->GetIntVal("cache", "PluginNetworkMode", 0)) != 0)
    {
        log_error ("init plugin manager failed.");
        return -1;
    }

    initPlugin = 1;

    return 0;
}

static int plugin_stop (void)
{
    plugin_mgr->close ();
    CPluginManager::Destroy ();
    plugin_mgr = NULL;

    return 0;
}

static int StatOpenFd()
{
	int count = 0;
	for(int i=0; i<1000; i++){
		if(fcntl(i, F_GETFL, 0) != -1)
			count++;
	}
	return count;
}

static int InitCacheMode()
{
    asyncUpdate = gConfig->GetIntVal("cache", "DelayUpdate", 0);
    if(asyncUpdate < 0 || asyncUpdate > 1)
    {
	log_crit("Invalid DelayUpdate value");
	return -1;
    }

    const char *keyStr = gConfig->GetStrVal("cache", "CacheShmKey");
    if(keyStr==NULL)
    {
	cacheKey = 0;
    }
    else if(!strcasecmp(keyStr, "none"))
    {
	log_notice("CacheShmKey set to NONE, Cache disabled");
	disableCache = 1;
    }
    else if(isdigit(keyStr[0]))
    {
	cacheKey = strtol(keyStr, NULL, 0);
    } else {
	log_crit("Invalid CacheShmKey value \"%s\"", keyStr);
	return -1;
    }

	disableDataSource = gConfig->GetIntVal("cache", "DisableDataSource", 0);
	if(disableCache && disableDataSource){
		log_crit("can't disableDataSource when CacheShmKey set to NONE");
		return -1;
	}

	if(disableCache && asyncUpdate){
		log_crit("can't DelayUpdate when CacheShmKey set to NONE");
		return -1;
	}
	
    if(disableCache==0 && cacheKey==0)
		log_notice("CacheShmKey not set, cache data is volatile");
	
	if(disableDataSource)
		log_notice("disable data source, cache data is volatile");
		
    return 0;
}

static int TTC_StartCacheThread()
{
		log_error("TTC_StartCacheThread start");
    cacheThread = new CPollThread("cache");
    cachePool = new CCacheProcess (cacheThread, gTableDef[0], asyncUpdate?MODE_ASYNC:MODE_SYNC);
    cachePool->SetLimitNodeSize(gConfig->GetIntVal ("cache", "LimitNodeSize", 100*1024*1024));
    cachePool->SetLimitNodeRows(gConfig->GetIntVal ("cache", "LimitNodeRows", 0));
    cachePool->SetLimitEmptyNodes(gConfig->GetIntVal ("cache", "LimitEmptyNodes", 0));

    if (cacheThread->InitializeThread () == -1)
    {
	return -1;
    }
    unsigned long long cacheSize = gConfig->GetSizeVal("cache", "CacheMemorySize", 0, 'M');
    if(cacheSize <= (50ULL << 20)) // 50M
    {
	    log_error("CacheMemorySize too small");
	    return -1;
    }

    else if(sizeof(long)==4 && cacheSize >= 4000000000ULL)
    {
	log_crit("CacheMemorySize %lld too large", cacheSize);
    } else if(
	    cachePool->cache_set_size(
		cacheSize,
		gConfig->GetIntVal("cache", "CacheShmVersion", 4)
		) == -1)
    {
	return -1;
    }

    /* disable async transaction log */
    cachePool->DisableAsyncLog(1);

    int lruLevel = gConfig->GetIntVal("cache", "DisableLRUUpdate", 0);
    if(disableDataSource) {
        if(cachePool->EnableNoDBMode() < 0) {
            return -1;
        }
        if(gConfig->GetIntVal("cache", "DisableAutoPurge", 0) > 0) {
            cachePool->DisableAutoPurge();
            // lruLevel = 3; /* LRU_WRITE */
        }
        int autoPurgeAlertTime = gConfig->GetIntVal("cache", "AutoPurgeAlertTime", 0);
        cachePool->SetDateExpireAlertTime(autoPurgeAlertTime);
        if (autoPurgeAlertTime > 0 &&gTableDef[0]->LastcmodFieldId()<=0)
        {
            log_crit("Can't start AutoPurgeAlert without lastcmod field");
            return -1;
        }
    }
    cachePool->DisableLRUUpdate(lruLevel);
    cachePool->EnableLossyDataSource(gConfig->GetIntVal("cache", "LossyDataSource", 0));

    if (asyncUpdate!=MODE_SYNC && cacheKey==0)
    {
	log_crit("Anonymous shared memory don't support DelayUpdate");
	return -1;
    }

    int iAutoDeleteDirtyShm = gConfig->GetIntVal("cache", "AutoDeleteDirtyShareMemory", 0);
    /*disable empty node filter*/
    if (cachePool->cache_open(cacheKey, 0, iAutoDeleteDirtyShm) == -1)
    {
	    return -1;
    }

    if(cachePool->update_mode() || cachePool->is_mem_dirty()) // asyncUpdate active
    {
	if(gTableDef[0]->UniqFields() < 1)
	{
	    log_crit("DelayUpdate needs uniq-field(s)");
	    return -1;
	}
	/*tapd:629601  by foryzhou*/
	if (disableDataSource)
	{
		if (cachePool->update_mode())
		{
			log_crit("Can't start async mode when disableDataSource.");
			return -1;
		}
		else
		{
			log_crit("Can't start disableDataSource with shm dirty,please flush async shm to db first or delete shm");
			return -1;
		}
	}
	else
	{
			if ((gTableDef[0]->CompressFieldId()>=0))
			{

				log_crit("sorry,TTC just support compress in disableDataSource mode now.");
				return -1;
			}
	}

	/*marker is the only source of flush speed calculattion, inc precision to 10*/
	cachePool->SetFlushParameter(
		gConfig->GetIntVal("cache", "MarkerPrecision", 10),
		gConfig->GetIntVal("cache", "MaxFlushSpeed", 1),
		gConfig->GetIntVal("cache", "MinDirtyTime", 3600),
		gConfig->GetIntVal("cache", "MaxDirtyTime", 43200)
		);

	cachePool->SetDropCount(
		gConfig->GetIntVal("cache", "MaxDropCount", 1000));
    } else {
		if(!disableDataSource)
			helperUnit->DisableCommitGroup();
    }

	if(cachePool->SetInsertOrder(dbConfig->ordIns) < 0)
			return -1;

		log_error("TTC_StartCacheThread end");
	return 0;
}

int StartRemoteClientThread()
{
		log_debug("StartRemoteClientThread begin");
		remoteThread = new CPollThread("remoteClient");
		remoteClient = new CDynamicHelperCollection(remoteThread, gConfig->GetIntVal("cache", "HelperCountPerGroup", 16));
		if (remoteThread->InitializeThread () == -1)
		{
				log_error("init remote thread error");
				return -1;
		}
	
    //get helper timeout
    int timeout = gConfig->GetIntVal("cache", "HelperTimeout", 30);
    int retry = gConfig->GetIntVal("cache", "HelperRetryTimeout", 1);
    int connect = gConfig->GetIntVal("cache", "HelperConnectTimeout", 10);

	remoteClient->SetTimerHandler(
					remoteThread->GetTimerList(timeout),
					remoteThread->GetTimerList(connect),
					remoteThread->GetTimerList(retry)
					);
		log_debug("StartRemoteClientThread end");
	return 0;
}

int TTC_StartDataSourceThread()
{
		log_debug("TTC_StartDataSourceThread begin");
		if(disableDataSource == 0){
				helperUnit = new CGroupCollect ();
				if (helperUnit->LoadConfig (dbConfig, gTableDef[0]->KeyFormat()) == -1)
				{
						return -1;
				}
		}

		//get helper timeout
		int timeout = gConfig->GetIntVal("cache", "HelperTimeout", 30);
		int retry = gConfig->GetIntVal("cache", "HelperRetryTimeout", 1);
		int connect = gConfig->GetIntVal("cache", "HelperConnectTimeout", 10);

		dsThread = new CPollThread("source");
		if (dsThread->InitializeThread () == -1)
				return -1;

		if(disableDataSource == 0)
				helperUnit->SetTimerHandler(
								dsThread->GetTimerList(timeout),
								dsThread->GetTimerList(connect),
								dsThread->GetTimerList(retry)
								);
		log_debug("TTC_StartDataSourceThread end");
		return 0;
}

static int TTC_Startup()
{
	if(TTC_StartDataSourceThread() < 0)
		return -1;
	if (StartRemoteClientThread() < 0)
		return -1;

	if(disableCache)
	{
		helperUnit->DisableCommitGroup();
	}
	else if(TTC_StartCacheThread() < 0)
		return -1;

	if(!disableDataSource)
		helperUnit->Attach(dsThread);

	int iMaxBarrierCount = gConfig->GetIntVal ("cache", "MaxBarrierCount", 1000);
	int iMaxKeyCount     = gConfig->GetIntVal ("cache", "MaxKeyCount", 1000);

	barCache = new CBarrierUnit (cacheThread?:dsThread, iMaxBarrierCount, iMaxKeyCount, CBarrierUnit::IN_FRONT);
	if(disableCache)
	{
		bypassUnit = new CCacheBypass(dsThread);
		barCache->BindDispatcher(bypassUnit);
		bypassUnit->BindDispatcher(helperUnit);
	} else {
		keyRoute = new CKeyRoute(cacheThread,gTableDef[0]->KeyFormat());
		if (!CheckAndCreate())
		{
			log_error("CheckAndCreate error");
			return -1;
		}
		else
			log_debug("CheckAndCreate ok");
		std::vector<ClusterNode> clusterConf;
		if (!ParseClusterConfig(&clusterConf))
		{
			log_error("ParseClusterConfig error");
			return -1;
		}
		else
			log_debug("ParseClusterConfig ok");

		keyRoute->Init(clusterConf);
		if(keyRoute->LoadNodeStateIfAny() != 0)
		{
			log_error("key route init error!");
			return -1;
		}
		log_debug("keyRoute->Init ok");
		barCache->BindDispatcher(keyRoute);
		keyRoute->BindCache(cachePool);
		keyRoute->BindRemoteHelper(remoteClient);
		cachePool->BindDispatcherRemote(remoteClient);
		if(disableDataSource){
			CBlackHole* hole = new CBlackHole(dsThread);
			cachePool->BindDispatcher(hole);
		}
		else{
			if(cachePool->update_mode() || cachePool->is_mem_dirty())
			{
				barHelper = new CBarrierUnit (dsThread, iMaxBarrierCount, iMaxKeyCount, CBarrierUnit::IN_BACK);
				cachePool->BindDispatcher(barHelper);
				barHelper->BindDispatcher(helperUnit);
			} else {
				cachePool->BindDispatcher(helperUnit);
			}
		}
	}

	serverControl = CServerControl::GetInstance(cacheThread?:dsThread);
	if(NULL == serverControl)
	{
		log_crit("create CServerControl object failed, errno[%d], msg[%s]", errno, strerror(errno));
		return -1;
	}
	serverControl->BindDispatcher(barCache);
	log_debug("bind server control ok");

/*
	listener = new CListenerPool;
	if(listener->Bind(gConfig, serverControl) < 0)
		return -1;
	log_debug("Bind listener ok");
*/

	agentProcess = new CAgentProcess(cacheThread?:dsThread);
        agentProcess->BindDispatcher(serverControl);

        agentListener = new CAgentListenPool();
        if(agentListener->Bind(gConfig, agentProcess) < 0)
               return -1;

	int open_cnt = StatOpenFd();
	gMaxConnCnt = gConfig->GetIntVal ("cache", "MaxFdCount", 1024) - open_cnt - 10; // reserve 10 fds
	if(gMaxConnCnt < 0){
		log_crit("MaxFdCount should large than %d", open_cnt + 10);
		return -1;
	}

	if(dsThread)
	{
		dsThread->RunningThread ();
	}

	if(remoteThread)
	{
		remoteThread->RunningThread ();
	}
	if(cacheThread)
	{
		cacheThread->RunningThread ();
	}

	agentListener->Run();

	return 0;
}

// second part of entry
static int main2(void *dummy);

int main (int argc, char *argv[])
{
    enable_memchecker();
    init_proc_title(argc, argv);    
    mkdir("../stat", 0777);
    mkdir("../data", 0777);
    if(TTC_DaemonInit (argc, argv) < 0)
        return -1;

    if(gConfig->GetIntVal ("cache", "EnableCoreDump", 0))
	DaemonEnableCoreDump();

    hashChanging = gConfig->GetIntVal("cache", "HashChanging", 0);
    targetNewHash = gConfig->GetIntVal("cache", "TargetNewHash", 0);

    CTTCGlobal::_pre_alloc_NG_num = gConfig->GetIntVal("cache", "PreAllocNGNum", 1024);
    CTTCGlobal::_pre_alloc_NG_num = CTTCGlobal::_pre_alloc_NG_num <=1 ? 1:
                                    CTTCGlobal::_pre_alloc_NG_num >=(1<<12)?1:
                                    CTTCGlobal::_pre_alloc_NG_num;

    CTTCGlobal::_min_chunk_size = gConfig->GetIntVal("cache", "MinChunkSize", 0);
    if (CTTCGlobal::_min_chunk_size < 0)
    {
        CTTCGlobal::_min_chunk_size = 0;
    }

    CTTCGlobal::_pre_purge_nodes = gConfig->GetIntVal("cache", "PrePurgeNodes", 0);
    if (CTTCGlobal::_pre_purge_nodes < 0)
    {
        CTTCGlobal::_pre_purge_nodes = 0;
    }
    else if (CTTCGlobal::_pre_purge_nodes > 10000)
    {
        CTTCGlobal::_pre_purge_nodes = 10000;
    }
	
	RELATIVE_HOUR_CALCULATOR->SetBaseHour(gConfig->GetIntVal("cache", "RelativeYear", 2014));

    InitStat();

    log_info("Table %s: key/field# %d/%d, keysize %d",
	    dbConfig->tblName,
	    gTableDef[0]->KeyFields(),
	    gTableDef[0]->NumFields()+1,
	    gTableDef[0]->MaxKeySize());

	if(InitCacheMode() < 0)
	    return -1;

    if (DaemonStart () < 0)
	return -1;

    CThread::SetAutoConfigInstance(gConfig->GetAutoConfigInstance("cache"));

	if (StartWatchDog(main2, NULL) < 0)
	    return -1;
    return main2(NULL);
}

static int main2(void *dummy)
{

    CThread *mainThread;
    NEW(CThread("main", CThread::ThreadTypeProcess), mainThread);
    if(mainThread != NULL) {
	    mainThread->InitializeThread();
    }

    if( DaemonSetFdLimit(gConfig->GetIntVal ("cache", "MaxFdCount", 0)) < 0)
	return -1;

    //start statistic thread. 
    statmgr.StartBackgroundThread();

    int ret = 0;
	ret = TTC_Startup();

    //gConfig->Dump(stdout, true);
    if(ret == 0)
    {
        extern void InitTaskExecutor(CTableDefinition *, CAgentListenPool *, CTaskDispatcher<CTaskRequest> *);
        InitTaskExecutor(gTableDef[0], agentListener, serverControl);

        //init plugin
        enablePlugin = gConfig->GetIntVal("cache", "EnablePlugin", 0); 
        if (enablePlugin)
        {
            if (plugin_start() < 0) 
            {
                return -1;
            }
        }

        if(getenv("TTCD_TEST_RUNNER")) {
            test_runner();
        }

#if MEMCHECK
        log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());

        report_mallinfo();
#endif
        log_info("%s v%s: running...", progname, version);
        DaemonWait ();
    }

    log_info("%s v%s: stoppping...", progname, version);
#if MEMCHECK
    log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());
    report_mallinfo();
#endif


    //stop plugin
    if (enablePlugin && initPlugin)
    {
        plugin_stop ();
    }

    DELETE(listener);

    if(cacheThread)
    {
        cacheThread->interrupt ();
    }

    if(dsThread)
    {
        dsThread->interrupt ();
    }

    if(remoteThread)
    {
        remoteThread->interrupt ();
    }
    extern void StopTaskExecutor(void);
    StopTaskExecutor();

    DELETE(cachePool);
    DELETE(helperUnit);
    DELETE(barCache);
	DELETE(keyRoute);
    DELETE(barHelper);
    DELETE(bypassUnit);
	DELETE(remoteClient);
    DELETE(cacheThread);
    DELETE(dsThread);
	DELETE(remoteThread);

    // CTaskPipe::DestroyAll(); // Don't delete ????
    statmgr.StopBackgroundThread();
    log_info("%s v%s: stopped", progname, version);
    DaemonCleanup();
#if MEMCHECK
    dump_non_delete();
    log_debug("memory allocated %lu virtual %lu", count_alloc_size(), count_virtual_size());
#endif
    return ret;
}

/* ends here */
