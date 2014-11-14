#ifndef __PLUGIN_TIMER_H__
#define __PLUGIN_TIMER_H__

#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/unistd.h>

#include "thread.h"

class CPluginTimer : public CThread
{
public:
	CPluginTimer (const char *name, const int timeout);
	virtual ~CPluginTimer ();

private:
	virtual void* Process(void);
	virtual int Initialize(void);
    virtual void interrupt (void);

private:
    dll_func_t*     _dll;
    const int       _interval;
    pthread_mutex_t _wake_lock;
};

#endif
