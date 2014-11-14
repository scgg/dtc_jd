#ifndef STAT_AGENT_H__
#define STAT_AGENT_H__

#include "StatInfo.h"
#include "StatManagerContainerThread.h"

extern TStatDefinition gStatDefinition[];
extern TStatDefinition gCheckaliveStatDefinition[];

CStatManager * CreateStatManager(uint32_t moduleId);
CStatManager * CreateCheckaliveStatManager();

#endif
