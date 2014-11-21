/*
 * =====================================================================================
 *
 *       Filename:  plugin_mgr.cc
 *
 *    Description:  实现TTC插件管理
 *
 *        Version:  1.0
 *        Created:  11/06/2009 11:37:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

//#include "config.h"
#include "plugin_agent_listener_pool.h"
#include "plugin_worker.h"
#include "plugin_agent_mgr.h"
#include "singleton.h"
#include "memcheck.h"
#include "log.h"
#include "plugin_agent_mgr.h"

//extern CConfig *gConfig;
typedef void (*so_set_network_mode_t) (void);
typedef void (*so_set_strings_t) (const char *);

CPluginAgentManager::CPluginAgentManager (void) :
    _dll                    (NULL),
    _config                 (NULL),
    _plugin_name            (NULL),
    _plugin_listener_pool   (NULL),
    _worker_notifier        (NULL),
    _worker_number          (1),
    _sb_if_handle           (NULL),
    _plugin_timer           (NULL),
    plugin_log_path			(NULL),
    plugin_log_name			(NULL),
    plugin_log_level		(0),
    plugin_log_size			(0),
    plugin_worker_number	(0),
    plugin_timer_notify_interval (0)
{

}

CPluginAgentManager::~CPluginAgentManager (void)
{

}

CPluginAgentManager* CPluginAgentManager::Instance (void)
{
    return CSingleton<CPluginAgentManager>::Instance();
}

void CPluginAgentManager::Destroy (void)
{
    return CSingleton<CPluginAgentManager>::Destroy ();
}

int CPluginAgentManager::open ()
{
    //create worker notifier
    NEW (worker_notify_t, _worker_notifier); 
    if (NULL == _worker_notifier)
    {
        log_error ("create worker notifier failed, msg:%s", strerror(errno));
        return -1;
    }

    if (load_plugin_depend() != 0)
    {
        return -1;
    }

    //create dll info
    _dll = (dll_func_t*) MALLOC (sizeof(dll_func_t));
    if (NULL == _dll)
    {
        log_error ("malloc dll_func_t object failed, msg:%s", strerror(errno));
        return -1;
    }
    memset (_dll, 0x00, sizeof(dll_func_t));

    if (NULL == _plugin_name)
    {
        log_error ("PluginName[%p] is invalid", _plugin_name);
        return -1;
    }

    //get plugin config file
    if (NULL == CPluginGlobal::_plugin_conf_file)
    {
        log_info ("PluginConfigFile[%p] is invalid", CPluginGlobal::_plugin_conf_file);
    }

    if (register_plugin (_plugin_name) != 0)
    {
        log_error ("register plugin[%s] failed", _plugin_name);
        return -1;
    }

    //invoke handle_init
    if (NULL != _dll->handle_init) 
    {
        const char* plugin_config_file[2] = {(const char*)CPluginGlobal::_plugin_conf_file, NULL};
        if (_dll->handle_init(1, (char**)plugin_config_file, PROC_MAIN) != 0)
        {
            log_error ("invoke plugin[%s]::handle_init() failed", _plugin_name);
            return -1;
        }
        log_debug("plugin::handle_init _plugin_conf_file=%s", (const char*)CPluginGlobal::_plugin_conf_file);
    }

    //crate CPluginAgentListenerPool object
    NEW (CPluginAgentListenerPool, _plugin_listener_pool);
    if (NULL == _plugin_listener_pool)
    {
        log_error ("create CPluginAgentListenerPool object failed, msg:%s", strerror(errno));
        return -1;
    }
    if (_plugin_listener_pool->Bind() != 0)
    {
        return -1;
    }

    //create worker pool
    if (create_worker_pool (plugin_worker_number) != 0)
    {
        log_error ("create plugin worker pool failed.");
        return -1;
    }

    //create plugin timer notify
    if (_dll->handle_timer_notify)
    {
        if (create_plugin_timer(plugin_timer_notify_interval) != 0)
        {
            log_error ("create usr timer notify failed.");
            return -1;
        }
    }

    return 0;
}

int CPluginAgentManager::close (void)
{
    //stop plugin timer
    if (_plugin_timer)
    {
        _plugin_timer->interrupt ();
        DELETE (_plugin_timer);
        _plugin_timer = NULL;
    }

    //stop worker
    _worker_notifier->Stop (PLUGIN_STOP_REQUEST);

    //destroy worker
    for (int i = 0; i < _worker_number; ++i)
    {
        if (NULL != _plugin_worker_pool[i])
        {
            _plugin_worker_pool[i]->interrupt ();
        }

        DELETE (_plugin_worker_pool[i]);
    }
    FREE (_plugin_worker_pool);
    _plugin_worker_pool = NULL;

    //delete plugin listener pool
    DELETE (_plugin_listener_pool);

    //unregister plugin
    if (NULL != _dll->handle_fini)
    {
        _dll->handle_fini (PROC_MAIN);
    }
    unregister_plugin ();
    FREE_CLEAR (_dll);

    //destroy work notifier
    DELETE (_worker_notifier);

    return 0;
}

int CPluginAgentManager::load_plugin_depend ()
{
    char* error     = NULL;
//    so_set_network_mode_t so_set_network_mode = NULL;
//    so_set_strings_t so_set_server_address  =   NULL;
//    so_set_strings_t so_set_server_tablename =  NULL;

    //load server bench so
    void* sb_if_handle = dlopen (SERVER_BENCH_SO_FILE, RTLD_NOW|RTLD_GLOBAL);
    if ((error = dlerror()) != NULL)
    {
        log_error ("so file[%s] dlopen error, %s", SERVER_BENCH_SO_FILE, error);
        return -1;
    }

//    //load ttc api so
//    void * ttcapi_handle = dlopen (TTC_API_SO_FILE, RTLD_NOW|RTLD_GLOBAL);
//    if ((error = dlerror()) != NULL)
//    {
//        log_error ("so file[%s] dlopen error, %s", TTC_API_SO_FILE, error);
//        return -1;
//    }

//    init plugin logger
//    const char* plugin_log_path = gConfig->GetStrVal ("cache", "PluginLogPath");
//    if (NULL == plugin_log_path)
//    {
//        plugin_log_path = (const char*)"../log";
//    }
//    const char* plugin_log_name = gConfig->GetStrVal ("cache", "PluginLogName");
//    if (NULL == plugin_log_name)
//    {
//        plugin_log_name = (const char*)"plugin_";
//    }
//    int plugin_log_level = gConfig->GetIdxVal("cache", "LogLevel", ((const char * const [])
//                           { "emerg", "alert", "crit", "error", "warning", "notice", "info", "debug", NULL }), 6);
//    int plugin_log_size = gConfig->GetIntVal("cache", "PluginLogSize", 1 << 28);

    DLFUNC (sb_if_handle, _sb_if_handle, plugin_log_init_t, "log_init");
    if (_sb_if_handle(plugin_log_path, plugin_log_level, plugin_log_size, plugin_log_name) != 0)
    {
        log_error ("init plugin logger failed.");
        return -1;
    }

    //set_network_mode
//    DLFUNC (ttcapi_handle, so_set_network_mode,so_set_network_mode_t,"set_network_mode");
//    if (so_set_network_mode&&mode)
//    {
//        so_set_network_mode();
//    }

    //set_server_address
//    DLFUNC (ttcapi_handle, so_set_server_address,so_set_strings_t,"set_server_address");
//    if (so_set_server_address)
//    {
//        log_debug("set server address:%s",gConfig->GetStrVal ("cache", "BindAddr"));
//        so_set_server_address(gConfig->GetStrVal ("cache", "BindAddr"));
//    }
    //set_server_tablename
//    DLFUNC (ttcapi_handle, so_set_server_tablename,so_set_strings_t,"set_server_tablename");
//    if (so_set_server_tablename)
//    {
//		    if (!gConfig->GetStrVal("TABLE_DEFINE", "TableName"))
//		    {
//			    log_error("can't find tablename in table.conf");
//			    return 0;
//		    }
//		    log_debug("set server tablename:%s",gConfig->GetStrVal("TABLE_DEFINE", "TableName"));
//		    so_set_server_tablename(gConfig->GetStrVal("TABLE_DEFINE", "TableName"));
//    }

    return 0;

out:
	return -1;
}

int CPluginAgentManager::register_plugin (const char* file_name)
{
    char*   error       = NULL;
    int     ret_code    = -1;

    _dll->handle = dlopen (file_name, RTLD_NOW|RTLD_NODELETE);
    if ((error = dlerror()) != NULL)
    {
        log_error ("dlopen error, %s", error);
        goto out;
    }

    DLFUNC_NO_ERROR (_dll->handle, _dll->handle_init,           handle_init_t,          "handle_init");
    DLFUNC_NO_ERROR (_dll->handle, _dll->handle_fini,           handle_fini_t,          "handle_fini");
    DLFUNC_NO_ERROR (_dll->handle, _dll->handle_open,           handle_open_t,          "handle_open");
    DLFUNC_NO_ERROR (_dll->handle, _dll->handle_close,          handle_close_t,         "handle_close");
    DLFUNC_NO_ERROR (_dll->handle, _dll->handle_timer_notify,   handle_timer_notify_t,  "handle_timer_notify");
    DLFUNC          (_dll->handle, _dll->handle_input,  handle_input_t,     "handle_input");
    DLFUNC          (_dll->handle, _dll->handle_process,handle_process_t,   "handle_process");

    ret_code = 0;

out:
    if (0 == ret_code)
    {
        log_info ("open plugin:%s successful.", file_name);
    }
    else
    {
        log_info ("open plugin:%s failed.", file_name);
    }

    return ret_code;
}

void CPluginAgentManager::unregister_plugin (void)
{
    if (NULL != _dll)
    {
        if (NULL != _dll->handle)
        {
            dlclose (_dll->handle);
        }

        memset (_dll, 0x00, sizeof(dll_func_t));
    }

    return;
}

int CPluginAgentManager::create_worker_pool (int worker_number)
{
    _worker_number = worker_number;

    if (_worker_number < 1 || _worker_number > CPluginGlobal::_max_worker_number)
    {
        log_warning ("worker number[%d] is invalid, default[%d]", _worker_number, CPluginGlobal::_default_worker_number);
        _worker_number = CPluginGlobal::_default_worker_number;
    }

    _plugin_worker_pool = (CThread**)calloc (_worker_number, sizeof(CThread*));
    if (NULL == _plugin_worker_pool)
    {
        log_error ("calloc worker pool failed, msg:%s", strerror(errno));
        return -1;
    }

    for (int i = 0; i < _worker_number; ++i)
    {
        char name[32];
        snprintf(name, sizeof(name) - 1, "plugin_worker_%d", i);

        NEW (CPluginWorkerThread(name, _worker_notifier, i), _plugin_worker_pool[i]);
        if (NULL == _plugin_worker_pool[i])
        {
            log_error ("create %s failed, msg:%s", name, strerror(errno));
            return -1;
        }

        if (_plugin_worker_pool[i]->InitializeThread() != 0)
        {
            log_error ("%s initialize failed.", name);
            return -1;
        }

        _plugin_worker_pool[i]->RunningThread ();
    }

    return 0;
}

int CPluginAgentManager::create_plugin_timer (const int interval)
{
    char name[32]               = {'\0'};
    int plugin_timer_interval   = 0;

    plugin_timer_interval = (interval < 1) ? 1 : interval;
    snprintf (name, sizeof(name) - 1, "%s", "plugin_timer");

    NEW (CPluginTimer(name, plugin_timer_interval), _plugin_timer);
    if (NULL == _plugin_timer)
    {
        log_error ("create %s failed, msg:%s", name, strerror(errno));
        return -1;
    }

    if (_plugin_timer->InitializeThread() != 0)
    {
        log_error ("%s initialize failed.", name);
        return -1;
    }

    _plugin_timer->RunningThread ();

    return 0;
}
