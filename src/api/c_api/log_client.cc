#include "ttcapi.h"
#include <log.cc>

__EXPORT
void TencentTableCache::write_log (int level,
		const char*file, const char *func, int lineno,
		const char *fmt, ...)
__attribute__((format(printf,5,6)))
__attribute__((__alias__("_write_log_")))
;

