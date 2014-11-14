/*
 * =====================================================================================
 *
 *       Filename:  blacklist_unit.h
 *
 *    Description:  给黑名单attach上执行单元，定期执行expired任务并输出统计值。
 *
 *        Version:  1.0
 *        Created:  03/17/2009 02:39:02 PM
 *       Revision:  $Id$
 *       Compiler:  gcc
 *
 *         Author:  jackda (ada), jackda@tencent.com
 *        Company:  TENCENT
 *
 * =====================================================================================
 */

#ifndef __TTC_BLACKLIST_UNIT_H
#define __TTC_BLACKLIST_UNIT_H

#include "namespace.h"
#include "timerlist.h"
#include "blacklist.h"
#include "log.h"
#include "StatTTC.h"

TTC_BEGIN_NAMESPACE

class CTimerObject;
class CBlackListUnit : public CBlackList, private CTimerObject
{
	public:
		CBlackListUnit(CTimerList *timer);
		virtual ~CBlackListUnit(void);

		int init_blacklist(const unsigned max, const unsigned type, const unsigned expired=10*60/*10 mins*/)
		{
			/* init statisitc item */
			stat_blacksize    = statmgr.GetSample(BLACKLIST_SIZE);
			stat_blslot_count = statmgr.GetItemU32(BLACKLIST_CURRENT_SLOT);

			return CBlackList::init_blacklist(max, type, expired);
		}

		int add_blacklist(const char *key, const unsigned vsize)
		{
			int ret = CBlackList::add_blacklist(key, vsize);
			if(0 == ret)
			{
				/* statistic */
				stat_blacksize.push(vsize);
				stat_blslot_count = current_blslot_count;
			}

			return ret;
		}

		int try_expired_blacklist(void)
		{
			int ret = CBlackList::try_expired_blacklist();
			if(0 == ret)
			{
				/* statistic */
				stat_blslot_count = current_blslot_count;
			}

			return ret;
		}

		void start_blacklist_expired_task(void)
		{
			log_info("start blacklist-expired task");
			
			AttachTimer(timer);

			return;
		}

	public:
		virtual void TimerNotify(void);

	private:
		CTimerList * timer;

		/* for statistic */
		CStatSample  stat_blacksize;  		/* black size distribution */
		CStatItemU32 stat_blslot_count;
};

TTC_END_NAMESPACE

#endif
