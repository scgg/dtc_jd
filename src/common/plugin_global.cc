/*
 * =====================================================================================
 *
 *       Filename:  plugin_global.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/09/2009 08:05:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>

#include "memcheck.h"
#include "plugin_global.h"

const char* CPluginGlobal::_internal_name[2]        = {(const char*)"ttc_internal", NULL};
const int   CPluginGlobal::_max_worker_number       = 1024;
const int   CPluginGlobal::_default_worker_number   = 1;
const int   CPluginGlobal::_max_plugin_recv_len     = 512;
int         CPluginGlobal::saved_argc               = 0;
char**      CPluginGlobal::saved_argv               = NULL;
char* 		CPluginGlobal::_plugin_conf_file        = NULL;
int			CPluginGlobal::_idle_timeout			= 10;
int			CPluginGlobal::_single_incoming_thread	= 0;
int			CPluginGlobal::_max_listen_count		= 256;
int			CPluginGlobal::_max_request_window		= 0;
char*		CPluginGlobal::_bind_address			= NULL;
int 		CPluginGlobal::_udp_recv_buffer_size	= 0;
int			CPluginGlobal::_udp_send_buffer_size	= 0;

void CPluginGlobal::dup_argv(int argc, char **argv) 
{
    saved_argc = argc;
    saved_argv = (char**)MALLOC (sizeof (char *) * (argc + 1));

    if (!saved_argv)
        return;

    saved_argv[argc] = NULL;

    while (--argc >= 0) {
        saved_argv[argc] = STRDUP (argv[argc]);
    }
}

void CPluginGlobal::free_argv(void)
{
    char **argv;

    for (argv = saved_argv; *argv; argv++)
        free (*argv);

    FREE_IF (saved_argv);
    saved_argv = NULL;
}
