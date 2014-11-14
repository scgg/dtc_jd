#ifndef __HB_COMM_H
#define __HB_COMM_H

#include <ttcapi.h>
#include <config.h>
#include <global.h>
#include <registor.h>

class CComm {
public:
	static void parse_argv(int argc, char **argv);
	static void show_usage(int argc, char **argv);
	static void show_version(int argc, char **argv);
	static int load_config();
	static int check_hb_status();
	static int check_table_define();
    static int fixed_hb_env();
	static int fixed_slave_env();
	static int connect_ttc_server(int ping_master);
	static int uniq_lock(const char *p = ASYNC_FILE_PATH);
	static int log_server_info(const char *p=ASYNC_FILE_PATH);

public:

	static TTC::Server master;
	static TTC::Server slave;
	static CConfig config;
	static CRegistor registor;
    static char * tablename;

	static const char *version;
	static int backend;
	static int fixed;
	static int purge;
	static int normal;
	static int skipfull;
};

#endif
