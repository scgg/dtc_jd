#ifndef __TTC_VERSION_H
#define __TTC_VERSION_H

#define TTC_MAJOR_VERSION       4
#define TTC_MINOR_VERSION       4
#define TTC_BETA_VERSION        0

/*major.minor.beta*/
#define TTC_VERSION "4.4.0"
/* the following show line should be line 11 as line number is used in Makefile */
#define TTC_GIT_VERSION "32e9a0b"
#define TTC_VERSION_DETAIL TTC_VERSION"."TTC_GIT_VERSION

extern const char compdatestr[];
extern const char comptimestr[];
extern const char version[];
extern const char version_detail[];
extern long       compile_time;

#endif
