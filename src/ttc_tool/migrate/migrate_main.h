/*
 * =====================================================================================
 *
 *       Filename:  migrate_main.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/13/2010 08:37:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _MIGRATE_MAIN_H_
#define _MIGRATE_MAIN_H_

#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "network.h"
#include "proccess_command.h"
#define HISTORY_PATH "../data/"

#ifndef SVN_REVISION
#error SVN_REVISION not defined!
#endif

#define STR(x)  #x
#define STR2(x) STR(x)
#define REVISION    "build " STR2(SVN_REVISION)

void show_usage(const char *prog);
void parse_args(int argc, char **argv);
int init_server(void);
void run_server(void);
void close_server(void);
#endif
