#ifndef __TTC_VERSION_H
#define __TTC_VERSION_H

#define TTC_MAJOR_VERSION       3
#define TTC_MINOR_VERSION       7
#define TTC_BETA_VERSION        1
/*major.minor.beta*/
#define TTC_VERSION "3.7.1"
#define TTC_VERSION_DETAIL TTC_VERSION"_release_0001"

extern const char compdatestr[];
extern const char comptimestr[];
extern const char version[];
extern const char version_detail[];
extern long       compile_time;

#endif
