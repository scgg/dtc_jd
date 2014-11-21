// Author: yizhiliu
// Date: 

#ifndef XBUFFER_H__
#define XBUFFER_H__

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <errno.h>

#include "noncopyable.h"

struct xbuffer : public noncopyable
{
    char *buf;
    uint32_t size;
    uint32_t wpos;  //write position
    uint32_t rpos;  //read position

    static const int XBUFFER_INIT_SIZE = 1024;

    xbuffer(uint32_t s = XBUFFER_INIT_SIZE) : size(s)
    {
        if(size == 0)
            size = XBUFFER_INIT_SIZE;
        buf = (char *)malloc(size);
        wpos = rpos = 0;
    }

    ~xbuffer() { free(buf); }

    void reset()
    {
        wpos = rpos = 0;
    }

    void ensure_capacity(uint32_t c)
    {
        if(capacity() >= c)
            return;

        //��������������Եõ��㹻�Ŀռ䣬��realloc
        if(rpos != 0 && size - wpos + rpos >= c)
        {
            tidy();
            return;
        }


        buf = (char *)realloc(buf, wpos + c);
        size = wpos + c;
    }

    void consume(uint32_t len)
    {
        rpos += len;
        if(rpos >= wpos)
        {
            rpos = wpos = 0;
        }
    }

    void gotbytes(uint32_t len)
    {
        wpos += len;
    }

    //����д��ĳ���
    uint32_t capacity()
    {
        return size - wpos;
    }

    char *writepos()
    {
        return buf + wpos;
    }

    char *readpos()
    {
        return buf + rpos;
    }

    //��δ���ĳ���
    uint32_t len()
    {
        return wpos -rpos;
    }

    void tidy()
    {
        if(rpos == 0)
            return;

        memmove(buf, buf + rpos, wpos - rpos);
        wpos -= rpos;
    }

    //��fd�ж�ȡ����
    //@retval 1 �ɹ���������buff�У����߶���EAGAIN
    //@retval 0 �Զ˹ر�����
    //@retval <0 ����-errno 
    int readfrom(int fd)
    {
        tidy();
        int rtn = read(fd, writepos(), capacity());
        if(rtn < 0)
        {
            if(errno == EAGAIN || errno == EINPROGRESS || errno == EINTR)
                return 1;

            return -errno;
        }

        if(rtn == 0)
            return 0;

        wpos += rtn;
        return 1;
    }

    int writeto(int fd)
    {
        int rtn = write(fd, readpos(), len());
        if(rtn < 0)
        {
            if(errno == EAGAIN || errno == EINPROGRESS || errno == EINTR)
                return 1;

            return -errno;
        }

        consume(rtn);
        return rtn;
    }
};



#endif

