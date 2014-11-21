// Author: yizhiliu
// Date: 

#ifndef SOCK_UTIL_H__
#define SOCK_UTIL_H__

#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/un.h>


inline void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

inline void set_reuseaddr(int fd)
{
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

inline void set_defer_accept(int s)
{
    int value = 1;
    setsockopt(s, IPPROTO_TCP, TCP_DEFER_ACCEPT, &value, sizeof(value));
}

//translate 127.0.0.1:9898 to sockaddr_in
//return 0 if str is a valid address
int string_to_sock_addr(const char *str, sockaddr_in *addr);
int listen_or_die(const sockaddr_un &addr, int backlog);
int listen_or_die(const sockaddr_in &addr, int backlog);


#endif

