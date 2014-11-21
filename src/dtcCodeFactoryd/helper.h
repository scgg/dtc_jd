#ifndef HELPER_H
#define HELPER_H

#include <time.h>
#include <sys/time.h>
#include <string>
#include <stdio.h>

static inline std::string GetCurrentFormatTime()
{
	time_t t = time(0);
    struct tm* tm_time = localtime(&t);

    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d",
        1900 + tm_time->tm_year, 1 + tm_time->tm_mon, tm_time->tm_mday,
        tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);

    return std::string(buf);
}

static inline long GetUnixTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long)tv.tv_sec;
}

#endif
