#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sched.h>

#include "plugin_agent_mgr.h"
#include "plugin_worker.h"
#include "plugin_global.h"
#include "memcheck.h"
#include "log.h"

CPluginWorkerThread::CPluginWorkerThread (const char *name, worker_notify_t* worker_notify, int seq_no) :
    CThread(name, CThread::ThreadTypeSync), _worker_notify (worker_notify), _seq_no (seq_no)
{
}

CPluginWorkerThread::~CPluginWorkerThread ()
{
}

int CPluginWorkerThread::Initialize(void)
{
    _dll = CPluginAgentManager::Instance()->get_dll ();
    if ((NULL == _dll) || (NULL == _dll->handle_init) || (NULL == _dll->handle_process))
    {
        log_error ("get server bench handle failed.");
        return -1;
    }

    return 0;
}

void CPluginWorkerThread::Prepare (void)
{
    const char* plugin_config_file[2] = {(const char*)CPluginGlobal::_plugin_conf_file, NULL};
    if (_dll->handle_init(1, (char**)plugin_config_file, _seq_no + PROC_WORK) != 0)
    {
        log_error ("invoke handle_init() failed, worker[%d]", _seq_no);
        return;
    }

    return;
}

void* CPluginWorkerThread::Process (void)
{
    plugin_request_t*   plugin_request  = NULL;

	while (!Stopping())
	{
        plugin_request = _worker_notify->Pop ();

        if (PLUGIN_STOP_REQUEST == plugin_request)
        {
            break;
        }

        if (NULL == plugin_request)
        {
            log_error ("the worker_notify::Pop() invalid, plugin_request=%p, worker[%d]", plugin_request, _gettid_());
            continue;
        }

		if (Stopping())
        {
			break;
        }

        plugin_request->handle_process ();
	}

    if (NULL != _dll->handle_fini)
    {
        _dll->handle_fini (_seq_no + PROC_WORK);
    }

	return NULL;
}
