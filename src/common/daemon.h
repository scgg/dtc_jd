#ifndef __TTC_DAEMON__H__
#define __TTC_DAEMON__H__

#define MAXLISTENERS	10

class CConfig;
struct CDbConfig;
class CTableDefinition;

extern CConfig *gConfig;
extern CDbConfig *dbConfig;
extern CTableDefinition *gTableDef[];

extern volatile int stop;
extern int background;

extern const char progname[];
extern const char version[];
extern const char usage_argv[];
extern char cacheFile[256];
extern char tableFile[256];
extern void ShowVersion(void);
extern void ShowUsage(void);
extern int TTC_DaemonInit(int argc, char **argv);
extern int BMP_DaemonInit(int argc, char **argv);
extern int DaemonStart(void);
extern void DaemonWait(void);
extern void DaemonCleanup(void);
extern int DaemonGetFdLimit(void);
extern int DaemonSetFdLimit(int maxfd);
extern int DaemonEnableCoreDump(void);
extern unsigned int ScanProcessOpenningFD(void);
#endif
