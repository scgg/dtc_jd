#ifndef _NETWORK_H_
#define _NETWORK_H_
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
    
#define MAX_PACKAGE_SIZE 1024*1024

inline void set_nonblocking(int fd)
{       
    fcntl(fd, F_SETFL, O_NONBLOCK);
}   

std::vector<std::string> split(const std::string &str, const char *delimeters);
sockaddr_in string_to_sockaddr(const std::string &str);
int safe_tcp_write(int sock_fd, void* szBuf, int nLen);
int safe_tcp_read(int sock_fd, void* szBuf, int nLen);
void send_result(int sock_fd,std::string &buf);
void send_result(int sock_fd,const char * buf);

#endif
