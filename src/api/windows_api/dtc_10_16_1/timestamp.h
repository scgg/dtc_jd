#ifndef __TTC_TIMESTAMP_H__
#define __TTC_TIMESTAMP_H__

#include <stdio.h>  
#include <time.h>  
#include <windows.h>  
  
#define SECS_TO_FT_MULT 10000000  
static LARGE_INTEGER base_time;  



typedef struct win_time_val  
{  
    /** The seconds part of the time. */  
    long    sec;  
  
    /** The miliseconds fraction of the time. */  
    long    msec;  
  
} win_time_val_t;  
  
typedef struct win_time  
{  
    /** This represents day of week where value zero means Sunday */  
    int wday;  
  
    /** This represents day of month: 1-31 */  
    int day;  
  
    /** This represents month, with the value is 0 - 11 (zero is January) */  
    int mon;  
  
    /** This represent the actual year (unlike in ANSI libc where 
     *  the value must be added by 1900). 
     */  
    int year;  
  
    /** This represents the second part, with the value is 0-59 */  
    int sec;  
  
    /** This represents the minute part, with the value is: 0-59 */  
    int min;  
  
    /** This represents the hour part, with the value is 0-23 */  
    int hour;  
  
    /** This represents the milisecond part, with the value is 0-999 */  
    int msec;  
  
}win_time_t;  
  
// Find 1st Jan 1970 as a FILETIME   
static void get_base_time(LARGE_INTEGER *base_time)  
{  
    SYSTEMTIME st;  
    FILETIME ft;  
  
    memset(&st,0,sizeof(st));  
    st.wYear=1970;  
    st.wMonth=1;  
    st.wDay=1;  
    SystemTimeToFileTime(&st, &ft);  
      
    base_time->LowPart = ft.dwLowDateTime;  
    base_time->HighPart = ft.dwHighDateTime;  
    base_time->QuadPart /= SECS_TO_FT_MULT;  
}  
  
int win_gettimeofday(win_time_val_t *tv)  
{  
    SYSTEMTIME st;  
    FILETIME ft;  
    LARGE_INTEGER li;  
   static char get_base_time_flag=0;  
  
    if (get_base_time_flag == 0)  
    {  
        get_base_time(&base_time);  
    }  
  
    /* Standard Win32 GetLocalTime */  
    GetLocalTime(&st);  
    SystemTimeToFileTime(&st, &ft);  
  
    li.LowPart = ft.dwLowDateTime;  
    li.HighPart = ft.dwHighDateTime;  
    li.QuadPart /= SECS_TO_FT_MULT;  
    li.QuadPart -= base_time.QuadPart;  
  
    tv->sec = li.LowPart;  
    tv->msec = st.wMilliseconds;  
  
    return 0;  
}  
  
int win_time(const win_time_val_t *tv, win_time_t *time)  
{  
    LARGE_INTEGER li;  
    FILETIME ft;  
    SYSTEMTIME st;  
  
    li.QuadPart = tv->sec;  
    li.QuadPart += base_time.QuadPart;  
    li.QuadPart *= SECS_TO_FT_MULT;  
  
    ft.dwLowDateTime = li.LowPart;  
    ft.dwHighDateTime = li.HighPart;  
    FileTimeToSystemTime(&ft, &st);  
  
    time->year = st.wYear;  
    time->mon = st.wMonth-1;  
    time->day = st.wDay;  
    time->wday = st.wDayOfWeek;  
  
    time->hour = st.wHour;  
    time->min = st.wMinute;  
    time->sec = st.wSecond;  
    time->msec = tv->msec;  
  
    return 0;  
}  






static inline int64_t  GET_TIMESTAMP(void) {

	win_time_val_t wintv;  
	win_time_t wintime;  
	win_gettimeofday(&wintv); 
	return (int64_t)wintv.sec*1000000+wintv.msec;

}






#define TIMESTAMP_PRECISION	1000000ULL

#endif
