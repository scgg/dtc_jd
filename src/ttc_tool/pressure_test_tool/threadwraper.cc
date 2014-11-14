#include "thread.h"

namespace common_utility
{

CThread::CThread()
{
	m_thread_handle = 0;
	m_is_attr_init = false;
}

CThread::~CThread()
{
	stop_thread();

#ifndef WIN32
	if (m_is_attr_init)
	{
		pthread_attr_destroy(&m_thread_attr);
	}
#endif
}

int CThread::launch_thread()
{
#ifdef WIN32
	m_thread_handle = _beginthreadex(0, 0, CThread::thread_routine, this, 0, 0);

	return 0;
#else
	int ret;

	ret = pthread_attr_init(&m_thread_attr);    

	if (0 != ret) 
	{      
		return -1;
	}

	m_is_attr_init = true;

	ret = pthread_attr_setstacksize(&m_thread_attr, (4 * 1024 * 1024));    

	if (0 != ret) 
	{      
		return -1;
	}
	
	ret = pthread_create(&m_thread_handle, &m_thread_attr, CThread::thread_routine, this);

	if (ret)
	{
		return -1;
	}	
#endif
	
	return 0;
}

#ifdef WIN32
unsigned int CThread::thread_routine(void* param)
#else
void *CThread::thread_routine(void* param)
#endif
{
#ifndef WIN32
	int last_state, last_type;
		
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &last_type);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &last_state);
#endif
	
	CThread* pThis = (CThread *)param;

	if (pThis)
	{
		pThis->run_loop();
	}

	return 0;
}

int CThread::run_loop()
{
	int iRet = 0;
	
	return iRet;
}
	void CThread::join_in()
	{
		pthread_join(m_thread_handle, 0);
		
	}
int CThread::stop_thread()
{
#ifndef WIN32
	void*	status;
	
	if (m_thread_handle)
	{
		pthread_cancel(m_thread_handle);
	}

	if (m_thread_handle)	
	{
		pthread_join(m_thread_handle, &status);
		m_thread_handle = 0;
	}
#endif

	return 0;
}

}
