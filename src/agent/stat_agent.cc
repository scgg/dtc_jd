#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>

#include "stat_agent.h"
#include "log.h"
#include "stat_definition.h"
#include "checkalive_stat_definition.h"


#define STATIDX "../stat/agent.%d.stat.idx"
#define CHECKALIVE_STATIDX "../stat/checkalive.stat.idx"
#define CHECKALIVE_MMAPNAME "checkalive"

TStatDefinition gStatDefinition[] = {
AGENT_STAT_DEF, { 0 } };

TStatDefinition gCheckaliveStatDefinition[]={
CHECKALIVE_STAT_DEF, { 0 } };

/*
 * 返回StatManager的指针
 */
CStatManager * CreateStatManager(uint32_t moduleId) {
	CStatManager *s = new CStatManager;
	char szModuleName[255];
	char szIndexName[255];
	snprintf(szModuleName, sizeof(szModuleName) - 1, "agent.%d", moduleId);
	snprintf(szIndexName, sizeof(szIndexName) - 1, STATIDX, moduleId);
	int ret = s->InitStatInfo(szModuleName, szIndexName, 0);
	if (ret == -1) {
		unlink(szIndexName);
		char buf[64];
		ret = s->CreateStatIndex(szModuleName, szIndexName, gStatDefinition,
				buf, sizeof(buf));
		if (ret != 0) {
			log_crit("CreateStatIndex failed: %s", s->ErrorMessage());
		}
		ret = s->InitStatInfo(szModuleName, szIndexName);
	}
	if (ret != 0) {
		//鍒濆鍖栧け璐ュ垹闄tatmanager
		log_crit("InitStatInfo failed: %s", s->ErrorMessage());
		delete s;
		return NULL;
	}
	else
	{
		return s;
	}
}

/*
 * 返回StatManager的指针
 */
CStatManager * CreateCheckaliveStatManager() {
	CStatManager *s = new CStatManager;
	int ret = s->InitStatInfo(CHECKALIVE_MMAPNAME,CHECKALIVE_STATIDX, 0);
	if (ret == -1) {
		unlink(CHECKALIVE_STATIDX);
		char buf[64];
		ret = s->CreateStatIndex(CHECKALIVE_MMAPNAME, CHECKALIVE_STATIDX, gCheckaliveStatDefinition,
				buf, sizeof(buf));
		if (ret != 0) {
			log_crit("CreateCheckaliveStatIndex failed: %s", s->ErrorMessage());
		}
		ret = s->InitStatInfo(CHECKALIVE_MMAPNAME, CHECKALIVE_STATIDX);
	}
	if (ret != 0) {
		//鍒濆鍖栧け璐ュ垹闄tatmanager
		log_crit("InitStatInfo failed: %s", s->ErrorMessage());
		delete s;
		return NULL;
	}
	else
	{
		return s;
	}
}

