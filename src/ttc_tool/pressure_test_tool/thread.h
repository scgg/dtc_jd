#ifndef _COMMON_UTILITY_THREAD_H_
#define _COMMON_UTILITY_THREAD_H_
#include "linuxheader.h"

namespace common_utility
{

class	CThread
{
public:
	int		launch_thread();
	int		stop_thread();
public:
	CThread();
	virtual	~CThread();
	void join_in();
private:
#ifdef WIN32
	static	unsigned int __stdcall thread_routine(void* param);
#else
	static	void*	thread_routine(void* param);
#endif
	virtual	int		run_loop();
private:
	bool			m_is_attr_init;
#ifdef WIN32
	uintptr_t       m_thread_handle;
#else
	pthread_attr_t 	m_thread_attr; 
	pthread_t 		m_thread_handle;
#endif
};

}
#endif

