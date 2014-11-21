/*
 * =====================================================================================
 *
 *       Filename:  sender.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/06/2010 06:38:04 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "sender.h"
#include "log.h"

CAGSender::CAGSender(int f, uint32_t maxQueueLen):
    fd(f),
    vec(NULL),
    pkt(NULL),
    nVec(0),
    currVec(0),
    sended(0),
    totalLen(0),
    leftLen(0),
    maxVecLen(maxQueueLen)
{
}

CAGSender::~CAGSender()
{
    for(uint32_t i = 0; i < currVec; i++)
    {
        if(pkt[i])
            free(pkt[i]);
    }
    if(vec)
        free(vec);
    if(pkt)
        free(pkt);
}

int CAGSender::Build()
{
    vec = (struct iovec *)malloc(sizeof(struct iovec) * 256);
    if(NULL == vec)
    {
        log_crit("no mem new struct iovec");
        return -1;
    }
    pkt = (void **)malloc(sizeof(void *) * 256);
    if(NULL == pkt)
    {
        log_crit("no mem new CAGPacket pointer array");
        return -1;
    }
    nVec = 256;
    return 0;
}

int CAGSender::AddPacket(const char *buf, int len)
{
    if(currVec == nVec)
    {
        if(nVec >= maxVecLen)
        {
            return -1;
        }

        vec = (struct iovec *)realloc((char *)vec, sizeof(struct iovec) * nVec * 2);
        pkt = (void **)realloc(pkt, sizeof(void *) * nVec * 2);
        nVec *= 2;
    }

    vec[currVec].iov_base = (void *)buf;
    vec[currVec].iov_len = len;
    pkt[currVec] = (void *)buf;
    currVec++;
    leftLen += len;

    return 0;
}

int CAGSender::Send()
{
    log_debug("CAGSender::Send start");
    if(0 == leftLen)
        return 0;

    int sd;
    struct msghdr msgh;
    struct iovec * v = vec;
    uint32_t cursor = 0;

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = vec;
    msgh.msg_iovlen = currVec;
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;
    msgh.msg_flags = 0;
    /* connection failed, sender need reconstructed */
    log_debug("the current vec is %u",currVec);
    sd = sendmsg(fd, &msgh, MSG_DONTWAIT|MSG_NOSIGNAL);
    if(sd < 0)
    {
        if(EINTR == errno || EAGAIN == errno || EINPROGRESS == errno)
            return 0;
        log_error("agent sender send error: %m, %d,free the buffer data", errno);
//        while(cursor<currVec)
//        {
//        	 free(pkt[cursor]);
//        	 cursor++;
//        }
//        currVec=0;
        return -1;
    }
#ifdef DEBUG
    log_info("send msg ok.");
#endif

    totalLen = leftLen;
    sended = sd;
    leftLen -= sd;

    if(0 == sd)
        return 0;

    while(cursor < currVec && sd >= (int)v->iov_len)
    {
        sd -= v->iov_len;
        free(pkt[cursor]);
        v++;
        cursor++;
    }

    if(sd > 0)
    {
        v->iov_base = (char *)v->iov_base + sd;
        v->iov_len -= sd;
    }
    int i;
    for(i = 0; cursor < currVec; cursor++, i++)
    {
        vec[i] = vec[cursor];
        pkt[i] = pkt[cursor];
    }
    currVec = i;	
    log_debug("CAGSender::Send stop");

    return 0;
}

