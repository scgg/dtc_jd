#ifndef __TTC_CONDITION_H__
#define __TTC_CONDITION_H__

#include "noncopyable.h"

class condition : private noncopyable
{
public:
    condition (void)
    {
	pthread_mutex_init(&m_lock, NULL);
	pthread_cond_init(&m_cond, NULL);
    }

    ~condition (void)
    {
	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_cond);
    }

    void notify_one (void)
    {
	pthread_cond_signal(&m_cond);
    }
    void notify_all (void)
    {
	pthread_cond_broadcast(&m_cond);
    }

    void wait(void)
    {
	pthread_cond_wait(&m_cond, &m_lock);
    }

    void lock(void)
    {
	pthread_mutex_lock(&m_lock);
    }

    void unlock(void)
    {
	pthread_mutex_unlock(&m_lock);
    }

private:
    pthread_cond_t m_cond;
    pthread_mutex_t m_lock;
};

class CScopedEnterCritical
{
public:
	CScopedEnterCritical(condition &c) : m_cond(c)
	{
		m_cond.lock();
	}

	~CScopedEnterCritical(void)
	{
		m_cond.unlock();
	}

private:
	condition & m_cond;
};

class CScopedLeaveCritical
{
public:
	CScopedLeaveCritical(condition &c) : m_cond(c)
	{
		m_cond.unlock();
	}

	~CScopedLeaveCritical(void)
	{
		m_cond.lock();
	}

private:
	condition & m_cond;
};

#endif //__TTC_CONDITION_H__


