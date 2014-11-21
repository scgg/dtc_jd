#ifndef PROBER_RESULT_SET_H
#define PROBER_RESULT_SET_H

#include "json/json.h"
#include <string>

enum {
	RET_CODE_DONE = 0,
	RET_CODE_NO_CMD,
	RET_CODE_INTERNAL_ERR,

	RET_CODE_CMD_NOT_SUPPORT = 101,
	RET_CODE_INIT_STAT_ERR,
	RET_CODE_INTT_MEM_CONF_ERR,
	RET_CODE_INIT_MEM_INFO_ERR,

	RET_CODE_BUSY = 201
};

struct ProberResultSet {
	int retcode;
	Json::Value resp;
	std::string errmsg;
};


#endif


