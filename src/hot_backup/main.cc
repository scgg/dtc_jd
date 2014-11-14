#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "registor.h"
#include "proctitle.h"
#include "sync_unit.h"
#include "daemon.h"
#include "comm.h"

int main(int argc, char **argv)
{
	init_proc_title(argc, argv);

	CComm::parse_argv(argc, argv);

	if (CComm::load_config((const char *)CComm::config_file)) {
		return 100;
	}

	_init_log_("hbp");
	_set_log_level_(CComm::config.GetIdxVal("SYSTEM", "LogLevel",
					      ((const char *const[]) {
					       "emerg",
					       "alert",
					       "crit",
					       "error",
					       "warning",
					       "notice",
					       "info", "debug", NULL}), 6));

	/* 锁住hbp的日志目录 */
	if (CComm::uniq_lock()) {
		log_error("another process already running, exit");
		return 100;
	}

	/* purge dirty key */
	if (CComm::purge) {
		if (CComm::connect_ttc_server(0)) {
			return 100;
		}

		CPurgeUnit purge_unit;

		if (purge_unit.Run(&CComm::master, &CComm::slave)) {
			return 100;
		}

		return 0;
	}

	/*  以 fix 模式启动，清理hb、slave环境 */
	if (CComm::fixed) {
		if (CComm::fixed_hb_env()) {
			return 100;
		}
		if (CComm::fixed_slave_env()) {
			return 100;
		}
	}

	if (CComm::check_hb_status()) {
		return 100;
	}

	if (CComm::connect_ttc_server(1)) {
		return 100;
	}

	if (CComm::registor.Init(&CComm::master, &CComm::slave)) {
		return 100;
	}

	CComm::log_server_info();

	DaemonBase::DaemonStart(CComm::backend);

	switch (CComm::registor.Regist()) {
		//增量同步
	case -TTC::EC_INC_SYNC_STAGE:
		{
			CIncSyncUnit inc_sync;

			if (inc_sync.Run(&CComm::master, &CComm::slave)) {
				return 100;
			}

			break;
		}

		//全量同步
	case -TTC::EC_FULL_SYNC_STAGE:
		{
            if (!CComm::skipfull)
            {
                CFullSyncUnit full_sync;
                if (full_sync.Run(&CComm::master, &CComm::slave,
                            CComm::config.GetIntVal("SYSTEM",
                                "SyncStep",
                                1000))) {
                    return 100;
                }
            }
			CIncSyncUnit inc_sync;
			if (inc_sync.Run(&CComm::master, &CComm::slave)) {
				return 100;
			}

			break;
		}

		//出错
	case -TTC::EC_ERR_SYNC_STAGE:
	default:
		{
			log_error("hb status is __NOT__ correct!"
				  "try use --fixed parament to start");
			return 100;
		}
	}

	DaemonBase::DaemonWait();

	return 0;
}