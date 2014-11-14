#include <sys/types.h>
#include <sys/socket.h>

#include "sock_util.h"

int send_n(int fd, char *buf, int len)
{
    int sent = 0;
    while(sent < len)
    {
        int rtn = send(fd, buf + sent, len - sent, 0);
        if(rtn < 0)
            return -1;

        sent += rtn;
    }

    return 0;
}

int recv_n(int fd, char *buf, int len)
{
    int recved = 0;
    while(recved < len)
    {
        int rtn = recv(fd, buf + recved, len - recved, 0);
        if(rtn <= 0)
            return -1;

        recved += rtn;
    }

    return 0;
}


