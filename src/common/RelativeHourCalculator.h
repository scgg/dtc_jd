#ifndef _RELATIVE_HOUR_CALCULATOR_H_
#define _RELATIVE_HOUR_CALCULATOR_H_

#include <string>
#include "singleton.h"
#include <sys/time.h>

class RelativeHourCalculator
{
	public:
		RelativeHourCalculator()
		{
			
		}
		~RelativeHourCalculator()
		{
		}
		/*由于mktimehelper原因，需要减去八个小时*/
		void SetBaseHour(uint64_t ddwBaseYear)
		{
			uint64_t ddwRelativeTime = MktimeHelper(ddwBaseYear,1,1,0,0,0);
			m_BaseHour = (ddwRelativeTime / 3600 - 8);
		}
		uint64_t GetRelativeHour()
		{
			 uint64_t ddwCurHour = time(NULL) / 3600;
			 
			return (ddwCurHour - m_BaseHour);
		}
		uint64_t GetBaseHour()
		{
			return m_BaseHour;
		}
		
		uint64_t MktimeHelper (uint64_t year, uint64_t mon,uint64_t day, uint64_t hour,uint64_t min, uint64_t sec)
		{
			if (0 >= (int) (mon -= 2))
			{ /**//* 1..12 -> 11,12,1..10 */
				mon += 12; /**//* Puts Feb last since it has leap day */
				year -= 1;
			}

			return ((((
				(unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
				year*365 - 719499
				)*24 + hour /**//* now have hours */
				)*60 + min /**//* now have minutes */
				)*60 + sec); /**//* finally seconds */
		}
	private:
		
		uint64_t m_BaseHour; /*本业务对应的ModuleId*/
		
};
#define RELATIVE_HOUR_CALCULATOR CSingleton<RelativeHourCalculator>::Instance()
#endif
