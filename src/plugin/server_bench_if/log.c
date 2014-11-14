#include <assert.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include "benchapi.h"

#define MAX_LOG_CNT	1000
#define LOG_BUF_SIZE	4096

static int has_init = 0;
static int log_level;
static u_int log_size;
static char log_pre[32] = {0};
static char log_dir[256] = {0};
static struct fds_t {
	int seq;
	int opfd;
	u_short day;
} fds_info[APP_TRACE + 1];
static char* log_buffer = MAP_FAILED;
static pthread_mutex_t log_buffer_lock = PTHREAD_MUTEX_INITIALIZER;
static inline int lock_buffer(void) { return  pthread_mutex_lock(&log_buffer_lock); }
static inline void unlock_buffer(void) { pthread_mutex_unlock(&log_buffer_lock); }

static inline void log_file_name(int lvl, int seq, char* file_name, time_t now)
{
	struct tm tm;

	assert (lvl >= APP_EMERG && lvl <= APP_TRACE);

	localtime_r(&now, &tm);
	snprintf (file_name, FILENAME_MAX, "%s/%s%s%04d%02d%02d%03d", log_dir, log_pre, 
					((char*[]){
						"emerg", "alert", "crit",
						"error", "warn", "notice",
						"info", "debug", "trace"
					})[lvl],
					tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, seq);
}

static int request_log_seq (int lvl)
{
	char file_name[FILENAME_MAX];
	int seq;
	time_t now = time (NULL);
	struct stat buf;
	/*
	 * find last log and tag with seq
	 */
	for (seq = 0; seq < MAX_LOG_CNT; seq ++) {
		log_file_name (lvl, seq, file_name, now);

		if (access (file_name, F_OK)) {
			if (seq == 0)
				return 0;
			else
				break;
		}
	}

	if (seq == MAX_LOG_CNT) {
		fprintf (stderr, "too many log files\n");
		return -1;
	}

	log_file_name (lvl, --seq, file_name, now);
	if (stat (file_name, &buf)) {
		 fprintf (stderr, "stat %s error, %m\n", file_name);
		 return -1;
	}
	
	return seq == 0 ? seq : seq - 1;
}

static int open_fd(int lvl, time_t now)
{
	char file_name[FILENAME_MAX];
	struct tm *tm;
	int val;

	log_file_name (lvl, fds_info[lvl].seq, file_name, now);
	fds_info[lvl].opfd = open (file_name, O_WRONLY|O_CREAT|O_APPEND, 0644);
	if (fds_info[lvl].opfd > 0) {
		tm = localtime (&now);
		fds_info[lvl].day = tm->tm_yday;
		
		val = fcntl(fds_info[lvl].opfd, F_GETFD, 0);
		val |= FD_CLOEXEC;
		fcntl(fds_info[lvl].opfd, F_SETFD, val);
	}
	return fds_info[lvl].opfd;
}

static int shift_fd (int lvl, time_t now)
{
	off_t length; 
	struct tm *tm;
	
	if (fds_info[lvl].opfd < 0 && unlikely (open_fd (lvl, now) < 0)) 
		return -1;

	length = lseek (fds_info[lvl].opfd, 0, SEEK_END);
	tm = localtime (&now);
	if (likely (length < log_size && fds_info[lvl].day == tm->tm_yday)) 
		return 0;

	close (fds_info[lvl].opfd);
	if ( fds_info[lvl].day != tm->tm_yday)
		fds_info[lvl].seq = 0;
	else
		fds_info[lvl].seq ++;

	return open_fd (lvl, now);
}

void boot_log (int OK, int dummy, const char* fmt, ...) {
}

static void write_log_unlocked (int lvl, const char* fmt, va_list ap)
{
	int unused;
	struct tm tm;
	int pos, end;
	time_t now;

	if (unlikely (!has_init)) {
		switch (lvl) {
			case APP_EMERG:
			case APP_CRIT:
			case APP_ALERT:
			case APP_ERROR:
				vfprintf (stderr, fmt, ap);
				break;
			default:
				vfprintf (stdout, fmt, ap);
				break;
		}
		return;
	}

	if (lvl > log_level)
		return ;

	now = time (NULL); 
	if (unlikely (shift_fd (lvl, now) < 0))
		return ;

	localtime_r(&now, &tm);
	if (lvl == APP_INFO || lvl == APP_NOTICE || lvl == APP_WARNING)
		pos = snprintf (log_buffer, LOG_BUF_SIZE, "[%02d:%02d:%02d]", 
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	else
		pos = snprintf (log_buffer, LOG_BUF_SIZE, "[%02d:%02d:%02d][%05d]", 
			tm.tm_hour, tm.tm_min, tm.tm_sec, getpid());
		
	
	end = vsnprintf (log_buffer + pos, LOG_BUF_SIZE-pos, fmt, ap);

	unused = write (fds_info[lvl].opfd, log_buffer, end + pos);
}

void sb_write_log (int lvl, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if(lock_buffer() != 0) return;
	write_log_unlocked(lvl, fmt, ap);
	unlock_buffer();
	va_end(ap);
}

int log_init (const char* dir, int lvl, u_int size, const char* pre_name)
{
	int i, ret_code = -1;

	if(has_init) {
		return 0;
	}

	if (lvl < 0 || lvl > APP_TRACE) {
		fprintf (stderr, "init log error, invalid log level=%d\n", lvl);
		goto exit_entry;
	}

	/*
	 * log file is no larger than 2GB
	 */
	if (size > 1<<31) {
		fprintf (stderr, "init log error, invalid log size=%d\n", size);
		goto exit_entry;
	}

	if (access (dir, W_OK)) {
		fprintf (stderr, "access log dir %s error, %m\n", dir);
		goto exit_entry;
	}

	if (log_buffer == MAP_FAILED) {
		log_buffer = mmap (0, 4096, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (log_buffer == MAP_FAILED) {
			fprintf (stderr, "mmap log buffer failed, %m\n");
			goto exit_entry;
		}
	}

	strncpy (log_dir, dir, sizeof (log_dir) - 1);
	if (pre_name != NULL)
		strncpy (log_pre, pre_name, sizeof (log_pre) - 1);
	log_size = size;
	log_level = lvl;

	for (i = APP_EMERG; i <= APP_TRACE; i++) {
		fds_info[i].opfd = -1;
		fds_info[i].seq = request_log_seq (i);

		if (fds_info[i].seq  < 0)
			goto exit_entry;
	}
	
	has_init = 1;
	ret_code = 0;
exit_entry:
	sb_write_log (ret_code==0?6:4, "set log dir %s, per file size %d MB, ret_code=%d", dir, size/1024/1024, ret_code);
	return ret_code;
}


