/*
 * =====================================================================================
 *
 *       Filename:  kd_syncer.h
 *
 *    Description:  key dump 
 *
 *        Version:  1.0
 *        Created:  08/30/2009 09:12:53 AM
 *       Revision:  $Id$
 *       Compiler:  g++
 *
 *         Author:  foryzhou (), foryzhou@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_KD_SYNCER_H
#define __TTC_KD_SYNCER_H

#include <stdint.h>
#include "ttcapi.h"
#include <fstream>
#define PRINT(fmt, args...) printf(fmt"\n", ##args)

/* output u64 format */
#if __WORDSIZE == 64
# define UINT64FMT "%lu"
#else
# define UINT64FMT "%llu"
#endif

#if __WORDSIZE == 64
# define INT64FMT "%ld"
#else
# define INT64FMT "%lld"
#endif

using namespace std;
namespace KD
{
	class kd_syncer
	{
		public:
			kd_syncer(void);
			virtual ~kd_syncer(void);

			int open(const char *addr,	 /* master address*/
					int step,	 /* sync step*/
					unsigned timeout,/* sync timeoud: s*/
					const char *filename /* key dump files name */
				);
			int run(void);
			int close(void);

		private:
			int detect_master_online();
			int sync_task_expired();
			int active_master();
			int save_data(TTC::Result &);
			char* get_current_time_str();

		private:
			ofstream	 fout; 		
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
