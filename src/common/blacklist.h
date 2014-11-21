/*
 * =====================================================================================
 *
 *       Filename:  blacklist.h
 *
 *    Description:  黑名单：用来屏蔽超大节点进入cache
 *
 *        Version:  1.0
 *        Created:  03/17/2009 08:00:50 AM
 *       Revision:  $Id$
 *       Compiler:  gcc
 *
 *         Author:  jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_BLACKLIST_H
#define __TTC_BLACKLIST_H

#include "list.h"
#include "hlist.h"
#include <stdint.h>
#include "namespace.h"

#define MAX_HASH_DEPTH	65535

TTC_BEGIN_NAMESPACE

struct blslot
{
	struct hlist_node hash; /* link to hash_list */
	struct list_head  time; /* link to time_list */
	unsigned int	  vsize;
	unsigned int	  expired;
	char 		  key[0];
}__attribute__((__aligned__(1)));

class CBlackList
{
	public:
		CBlackList();
		~CBlackList();

		int init_blacklist(const unsigned max, const unsigned type, const unsigned expired);
		int add_blacklist(const char *packedkey, const unsigned vsize);
		int in_blacklist( const char *packedkey);

		/* dump all blslot in blacklist, debug only */	
		void dump_all_blslot(void);
	private:
		void stat_everything(const struct blslot *, const int add);

	protected:
		/* try expire all expired slot */
		int try_expired_blacklist(void);

	protected:
		unsigned current_blslot_count;

	private:
		/* double linked hash list with single pointer list head */
		struct hlist_head hash_list[MAX_HASH_DEPTH];

		/* time list */
		struct list_head time_list;

		unsigned max_blslot_count;
		unsigned blslot_expired_time;
		unsigned key_type;
};

TTC_END_NAMESPACE
#endif
