#ifndef __H_CACHE_FLUSH_H__
#define __H_CACHE_FLUSH_H__

#include "timerlist.h"
#include "lqueue.h"
#include "task_request.h"
#include "cache_process.h"
#include "log.h"

class CCacheProcess;

class CFlushRequest
{
private:
    CCacheProcess *owner;
    // char *key;
    int numReq;
    int badReq;
    CTaskRequest *wait;

public:
    friend class CCacheProcess;
    CFlushRequest(CCacheProcess *, const char *key);
    ~CFlushRequest(void);

    const CTableDefinition *TableDefinition(void) const { return owner->TableDefinition(); }

    int FlushRow(const CRowValue &);
    void CompleteRow(CTaskRequest *req, int index);
    int Count(void) const { return numReq; }
};

#endif

