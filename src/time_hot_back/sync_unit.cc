#include <unistd.h>
#include <sys/types.h>
#include "comm.h"
#include "sync_unit.h"

/**
 * @brief fork两个进程，一个同步key，一个同步value
 * 
 * @param 
 * 
 * @return 
 * 
 * @modify 17/11/08 12:23:52 jackda Create
 **/

int CIncSyncUnit::Run(TTC::Server * m, TTC::Server * s)
{
	log_notice("\"INC-SYNC\" is start");

	/* 先关闭连接，防止fd重路 */
	m->Close();
	s->Close();

	/* start SyncKey */
	pid_t pid =::fork();
	if (pid < 0) {
		log_error("fork SyncKey process failed, %m");
		return -1;

	} else if (pid == 0) {
		set_proc_title("hbp[SyncKey]");
		set_proc_name("hbp[SyncKey]");

		NEW(CIncSyncKey, _inc_sync_k);

		if (!_inc_sync_k) {
			log_error
			    ("hbp[SyncKey]: create CIncSyncKey obj failed");
			return -1;
		}

		if (_inc_sync_k->Init(m)) {
			log_error
			    ("hbp[SyncKey]: init CIncSyncKey obj failed, %s",
			     _inc_sync_k->ErrorMessage());
			return -1;
		}

		/* 设置批量拉取更新key的limit默认为1000 */
		_inc_sync_k->SetLimit(CComm::config.GetIntVal("SYSTEM", "SyncStep", 1000));

		log_notice("hbp[SyncKey] process started");

		_inc_sync_k->Run();

		log_notice("hbp[SyncKey] process stoped");

		exit(0);
	}

	/* 小睡一下，等SyncKey进程构建环境 */
	sleep(2);

	/* start SyncValue */
	pid =::fork();
	if (pid < 0) {
		log_error("fork SyncValue process failed, %m");
		return -1;

	} else if (pid == 0) {
		set_proc_title("hbp[SyncValue]");
		set_proc_name("hbp[SyncValue]");

		NEW(CIncSyncValue, _inc_sync_v);

		if (!_inc_sync_v) {
			log_error
			    ("hbp[SyncValue]: create CIncSyncValue obj failed");
			return -1;
		}

		if (_inc_sync_v->Init(m, s)) {
			log_error
			    ("hbp[SyncValue]: init CIncSyncValue obj failed, %s",
			     _inc_sync_v->ErrorMessage());
			return -1;
		}

		log_notice("hbp[SyncValue] process started");

		_inc_sync_v->Run();

		log_notice("hbp[SyncValue] process stoped");

		exit(0);
	}

	return 0;
}
