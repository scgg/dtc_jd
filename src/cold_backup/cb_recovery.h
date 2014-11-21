/*
 * =====================================================================================
 *
 *       Filename:  cb_recovery.h
 *
 *    Description:  recovery log to mem
 *
 *        Version:  1.0
 *        Created:  05/26/2009 10:29:49 AM
 *       Revision:  $Id: cb_recovery.h 1842 2009-05-26 08:38:10Z jackda $
 *       Compiler:  gcc
 *
 *         Author:  jackda (), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_CB_RECOVERY
#define __TTC_CB_RECOVERY

#include "ttcapi.h"
#include "cb_value.h"


#define PRINT(fmt, args...) printf(fmt"\n", ##args)

namespace CB
{
	template <class T>
	class cb_recovery
	{
		public:
			cb_recovery();
			virtual ~cb_recovery();

			int open(const char *address, const char *backupdir);
			int run();
			int close();
			
		private:
			int active_recovery_machine(void);
			int recovery_to_slave(CBinary key, CBinary value);
			char *get_current_time_str();

		private:
			T		_logr;
			TTC::Server 	_recovery_machine;
			int		_continous_error_count;
			uint64_t	_recovery_node_count;
	};
};

#endif
