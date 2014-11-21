#ifndef _COMMON_UTILITY_THREAD_POOL_H_
#define _COMMON_UTILITY_THREAD_POOL_H_
#include "thread.h"
#include <ctype.h>
#include <map>

namespace common_utility
{
	template <class T>
	class	CMultiThread
	{
		class CMultiThreadStub : public CThread
		{
			public:
				CMultiThreadStub(){}
				int init(CMultiThread<T> *thread, int thread_id){
					m_multi_thread = thread;
					m_thread_id = thread_id;
				}
			virtual	int		run_loop(){
				if (m_multi_thread)
					m_multi_thread->run(m_thread_id);
			}
			private:
				int m_thread_id;
				CMultiThread<T> *m_multi_thread;
		};
		typedef int (T::*THREADROUTINE)(int thread_id, int thread_param);
	public:
		CMultiThread(){
			m_thread_routine = NULL;
			m_is_init = false;
		}
		virtual	~CMultiThread(){
		}
	public:
		int init(T *pThis, THREADROUTINE thread_routine, int num_threads, int muti_thread_param){
			m_pThis = pThis;
			m_thread_routine = thread_routine;
			m_num_thread = num_threads;
			m_muti_thread_param = muti_thread_param;
			m_threads = new CMultiThreadStub[num_threads];

			for (int i = 0; i < num_threads; i++)
			{
				m_threads[i].init(this, i);
			}
			m_is_init = true;
		}
		int start(){
			if (!m_is_init)
			{
				return -1;
			}

			for (int i = 0; i < m_num_thread; i++)
			{
				m_threads[i].launch_thread();
			}
			
			return 0;
		}
		
		int stop(){
			for (int i = 0; i < m_num_thread; i++)
			{
				m_threads[i].stop_thread();
			}
		}
		
		void join_in()
		{
			for (int i = 0; i < m_num_thread; i++)
			{
				m_threads[i].join_in();
			}
		}
		int	run(int thread_id){
			if (m_thread_routine && m_pThis)
				(m_pThis->*m_thread_routine)(thread_id, m_muti_thread_param);
		}
	public:
		CMultiThreadStub *m_threads;
	private:
		bool          m_is_init;
		THREADROUTINE m_thread_routine;
		int           m_muti_thread_param;
		int           m_num_thread;
		T            *m_pThis;
	};
}
#endif

