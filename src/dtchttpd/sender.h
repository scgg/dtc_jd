/*
 * =====================================================================================
 *
 *       Filename:  ag_sender.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/06/2010 06:36:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __AG_SENDER_H__
#define __AG_SENDER_H__

#include <stdint.h>
#include <sys/types.h>

class CAGSender
{
private:
    int fd;
    struct iovec * vec;
    void **pkt;
    uint32_t nVec;
    uint32_t currVec;
    uint32_t sended;
    uint32_t totalLen;
    uint32_t leftLen;
    uint32_t maxVecLen;

public:
    CAGSender(int f, uint32_t maxQueueLen);
    virtual ~CAGSender();

    int Build();
    int AddPacket(const char *buf, int len);
    int Send();
    inline uint32_t Sended() { return sended; }
    inline uint32_t TotalLen() { return totalLen; }
    inline uint32_t LeftLen() { return leftLen; }
    inline bool SendDone() { return leftLen == 0; }
};

#endif
