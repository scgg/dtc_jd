#ifndef EPOLL_REACTOR_H__
#define EPOLL_REACTOR_H__

#include <sys/epoll.h>
#include <vector>

#include "event_handler.h"

typedef void (*pfn_action)(void *arg);

class epoll_reactor
{
public:
    int attach(int fd, event_handler *eh, int events);
    int modify(int fd, event_handler *eh, int events);
    int detach(int fd);
    void run();
    int run_once();
    void stop() { m_stop = 1; }
    bool stoped() { return m_stop == 1; }
    void push_delay_task(pfn_action task, void *arg);

    epoll_reactor(int size);
    ~epoll_reactor();
    
private:
    int m_epoll;
    volatile int m_stop;
    std::vector<epoll_event> m_events;
    
    struct delay_task
    {
        pfn_action function;
        void *arg;
    };

    std::vector<delay_task> m_tasks;
    epoll_reactor(const epoll_reactor &);
    epoll_reactor &operator =(const epoll_reactor &);
};

#endif
