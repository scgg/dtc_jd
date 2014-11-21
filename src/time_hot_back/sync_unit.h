/**
 * @brief  增量同步由两个进程完成。
 *
 * @file sync_unit.h
 * @author jackda
 * @time 17/11/08 11:56:28
 * @version 0.1
 *
 * CopyRight (C) jackda(答治茜) @2007~2010@
 *
 **/

#ifndef __SYNC_UNIT_H
#define __SYNC_UNIT_H

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "daemon.h"
#include "proctitle.h"
#include "full_sync.h"
#include "incsync_key.h"
#include "incsync_value.h"

class DaemonBase;

class CFullSyncUnit {
public:

	CFullSyncUnit():_full_sync(0) {
	} ~CFullSyncUnit() {
		DELETE(_full_sync);
	}

	int Run(TTC::Server * m, TTC::Server * s, int limit) {
		log_notice("\"FULL-SYNC\" is start");

		NEW(CFullSync(m, s), _full_sync);
		if (!_full_sync) {
			log_error
			    ("full-sync is __NOT__ complete, plz fixed, err: create CFullSync obj failed");

			return -1;
		}

		if (_full_sync->Init()) {
			log_error
			    ("full-sync is __NOT__ complete, plz fixed, err: %s",
			     _full_sync->ErrorMessage());
			return -1;
		}

		_full_sync->SetLimit(limit);

		if (_full_sync->Run()) {
			log_error
			    ("full-sync is __NOT__ complete, plz fixed, err: %s",
			     _full_sync->ErrorMessage());

			return -1;
		}

		log_notice("\"FULL-SYNC\" is stop");

		return 0;

	}

private:
	CFullSync * _full_sync;
};

class CPurgeUnit {
public:
	CPurgeUnit():_purge_key(0) {
	} ~CPurgeUnit() {
		DELETE(_purge_key);
	}

	int Run(TTC::Server * m, TTC::Server * s) {
		log_notice("purge-dirty-key is start");

		NEW(CIncSyncValue, _purge_key);

		if (!_purge_key) {
			log_error("purge dirty key is __NOT__ complete, \
                                err: create CIncSyncValue obj failed");

			return -1;
		}

		if (_purge_key->Init(m, s)) {
			log_error
			    ("purge dirty key is __NOT__ complete, err: %s",
			     _purge_key->ErrorMessage());

			return -1;
		}

		_purge_key->PurgeAllLocalDirtyKeys();

		log_notice("purge-dirty-key is stop");

		return 0;

	}

private:
	CIncSyncValue * _purge_key;

};

class CIncSyncUnit {
public:
	CIncSyncUnit():_inc_sync_k(0), _inc_sync_v(0) {
	} ~CIncSyncUnit() {
		DELETE(_inc_sync_k);
		DELETE(_inc_sync_v);
	}

	int Run(TTC::Server * m, TTC::Server * s);

private:
	CIncSyncKey * _inc_sync_k;	/* 同步key */
	CIncSyncValue *_inc_sync_v;	/* 同步value */
};

#endif
