#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "compiler.h"
#include "log.h"

#define LOGSIZE 4096

int __log_level__ = 6;

static int noconsole = 0;
static int logfd = -1;
static int logday = 0;
static char log_dir[128] = "../log";
static char appname[32] = "";
static int (*alert_hook)(const char *, int);
#if HAS_TLS
static __thread const char *threadname __TLS_MODEL;
void _set_log_thread_name_(const char *n) { threadname = n; }
#else
static pthread_key_t namekey;
static pthread_once_t nameonce = PTHREAD_ONCE_INIT;
static void _init_namekey_(void) { pthread_key_create(&namekey, NULL); }
void _set_log_thread_name_(const char *n) { pthread_setspecific(namekey, n); }
#endif

#include "StatTTC.h"
CStatItemU32 *__statLogCount;
unsigned int __localStatLogCnt[8];

// clean logfd when module unloaded
__attribute__((__destructor__))
static void clean_logfd(void) {
	if(logfd >= 0) {
		close(logfd);
		logfd = -1;
	}
}

void _init_log_ (const char *app, const char *dir)
{
#if !HAS_TLS
	pthread_once(&nameonce, _init_namekey_);
#endif
	__statLogCount = NULL; // 暂时只在本地统计
	memset(__localStatLogCnt, 0, sizeof(__localStatLogCnt));

	strncpy(appname, app, sizeof(appname)-1);
	
	if(dir) {
		strncpy (log_dir, dir, sizeof (log_dir) - 1);
	}
	mkdir(log_dir, 0777);
	if(access(log_dir, W_OK|X_OK) < 0)
	{
		log_error("logdir(%s): Not writable", log_dir);
	}

	logfd = open("/dev/null", O_WRONLY);
	if(logfd < 0)
		logfd = dup(2);
	fcntl(logfd, F_SETFD, FD_CLOEXEC);
}

void _set_log_level_(int l)
{
	if(l>=0)
		__log_level__ = l > 4 ? l : 4;
}

void _set_log_alert_hook_(int(*alert_func)(const char*, int))
{
	alert_hook = alert_func;
}

void _write_log_(
	int level, 
	const char *filename, 
	const char *funcname,
	int lineno,
	const char *format, ...)
{
	// save errno
	int savedErrNo = errno;
	int off = 0;
	int msgoff = 0;
	char buf[LOGSIZE];
	char logfile[256];
#if !HAS_TLS
	const char *threadname;
#endif

	if(appname[0] == 0)
		return;

	if(level < 0) level = 0;
	else if(level > 7) level = 7;
	if(__statLogCount==NULL)
		++__localStatLogCnt[level];
	else
		++__statLogCount[level];
	
	// construct prefix
	struct tm tm;
	time_t now = time (NULL);
	localtime_r(&now, &tm);
#if HAS_TLS
#else
	pthread_once(&nameonce, _init_namekey_);
	threadname = (const char *)pthread_getspecific(namekey);
#endif
	if(filename==NULL) {
		if(threadname) {
			off = snprintf (buf, LOGSIZE,
					"<%d>[%02d:%02d:%02d] %s: ",
					level,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					threadname
				       );
		} else {
			off = snprintf (buf, LOGSIZE,
					"<%d>[%02d:%02d:%02d] pid[%d]: ",
					level,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					_gettid_()
				       );
		}
	} else {
		filename = basename(filename);
		if(threadname)
			off = snprintf (buf, LOGSIZE,
					"<%d>[%02d:%02d:%02d] %s: %s(%d)[%s]: ",
					level,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					threadname,
					filename, lineno, funcname
				       );
		else
			off = snprintf (buf, LOGSIZE,
					"<%d>[%02d:%02d:%02d] pid[%d]: %s(%d)[%s]: ",
					level,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					_gettid_(),
					filename, lineno, funcname
				       );
	}

	if(off >= LOGSIZE)
		off = LOGSIZE - 1;
		
	{
	int today = tm.tm_year*1000 + tm.tm_yday;
	if(logfd >= 0 && today != logday)
	{
		int fd;

		logday = today;
		snprintf (logfile, sizeof(logfile),
				"%s/%s.error%04d%02d%02d.log", log_dir, appname,
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
		fd = open (logfile, O_CREAT | O_LARGEFILE | O_APPEND |O_WRONLY, 0644);
		if(fd >= 0)
		{
			dup2(fd, logfd);
			close(fd);
			fcntl(logfd, F_SETFD, FD_CLOEXEC);
		}
	}
	}

	// remember message body start point
	msgoff = off;

	{
		// formatted message
		va_list ap;
		va_start(ap, format);
		// restore errno
		errno = savedErrNo;
		off += vsnprintf(buf+off, LOGSIZE-off, format, ap);
		va_end(ap);
	}

	if(off >= LOGSIZE)
		off = LOGSIZE - 1;
	if(buf[off-1]!='\n'){
		buf[off++] = '\n';
	}

	int unused;

	if(logfd >= 0) {
		unused = write(logfd, buf, off);
	}

	if(level <= 6 && !noconsole) {
		// debug don't send to console/stderr
		unused = fwrite(buf+3, 1, off-3, stderr);
		if(unused <= 0) {
			// disable console if write error
			noconsole = 1;
		}
	}

	if(alert_hook && level <= 1 /* emerg,alert */) {
		if(alert_hook(buf+msgoff, off-msgoff-1) < 0) {
			// attr report error, log a warning message
			buf[1] = '4'; // 4 is warning level
			// replace message body
			off = msgoff + snprintf(buf+msgoff, LOGSIZE-msgoff, "%s", "report to attr failed\n");
			// log another line
			unused = fwrite(buf+3, 1, off-3, stderr);
			if(logfd >= 0) {
				unused = write(logfd, buf, off);
			}
		}
	}
}

