/*
 * =====================================================================================
 *
 *       Filename:  plugin_mgr.h
 *
 *    Description:  plugin manager
 *
 *        Version:  1.0
 *        Created:  11/06/2009 10:19:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef __TTC_PLUGIN_MANAGER_H__
#define __TTC_PLUGIN_MANAGER_H__

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "noncopyable.h"
#include "plugin_request.h"
#include "plugin_timer.h"

class CConfig;
class CPluginListenerPool;
class CPluginWorkerThread;

class CPluginManager : private noncopyable
{
public:
    CPluginManager (void);
    ~CPluginManager (void);

    static CPluginManager* Instance (void);
    static void Destroy (void);

    int open (int mode);
    int close (void);

    inline dll_func_t* get_dll (void)
    {
        return _dll;
    }

    inline worker_notify_t* get_worker_notifier (void)
    {
        return _worker_notifier;
    }

public:

protected:
protected:

private:
    int register_plugin (const char *file_name);
    void unregister_plugin (void);
    int create_worker_pool (int worker_number);
    int load_plugin_depend (int mode);
    int create_plugin_timer (const int interval);

private:
    dll_func_t*             _dll;
    CConfig*                _config;
    const char*             _plugin_name;
    CPluginListenerPool*    _plugin_listener_pool;
    CThread**               _plugin_worker_pool;
    worker_notify_t*        _worker_notifier;
    int                     _worker_number;
    plugin_log_init_t       _sb_if_handle;
    CThread*                _plugin_timer;
};

#endif
