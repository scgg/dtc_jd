/*
 * =====================================================================================
 *
 *       Filename:  cb_syncer.h
 *
 *    Description:  cold backup syncer implementation
 *
 *        Version:  1.0
 *        Created:  05/13/2009 12:19:50 AM
 *       Revision:  $Id: cb_syncer.h 1842 2009-05-26 08:38:10Z jackda $
 *       Compiler:  gcc
 *
 *         Author:  jackda (ada), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_SYNCER_H
#define __TTC_CB_SYNCER_H

#include <stdint.h>
#include <stdio.h>
#include "ttcapi.h"

#define PRINT(fmt, args...) printf(fmt"\n", ##args)

namespace CB
{
	template <typename T>
	class cb_syncer
	{
		public:
			cb_syncer(void);
			virtual ~cb_syncer(void);

			int open(const char *addr,	 /* master address*/
					int step,	 /* sync step*/
					unsigned timeout,/* sync timeoud: s*/
					const char *dir, /* log dir */
					size_t slice     /* log file slice size */);
			int run(void);
			int close(void);

		private:
			int detect_master_online();
			int sync_task_expired();
			int active_master();
			int save_data(TTC::Result &);
			char* get_current_time_str();

		private:
			T		_logw; 		/* logger */
			TTC::Server	_master;
			int 		_step;
			time_t		_start;
			time_t		_timeout;
			uint64_t 	_sync_node_count_stat;
			char 		_master_addr[128];
			char		_errmsg[256];
	};
};

#endif
