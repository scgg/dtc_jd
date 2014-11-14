/*
 * =====================================================================================
 *
 *       Filename:  receiver.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/31/2010 12:40:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef __AG_RECEIVER_H__
#define __AG_RECEIVER_H__

#include <stdint.h>

#include "protocol.h"

class CAGReceiver
{
private:
    char m_header[sizeof(CPacketHeader)];
    char *m_buf;
    uint32_t m_bufSize;
    uint32_t m_offset;
    int m_fd;
public:
    CAGReceiver(int f);
    ~CAGReceiver();
    void GetPacket(char **buf, uint32_t *len);
    /**
     * 0: normal case
     * -1: error or connection closed
     * 1: got a packet
     */
    int Recv();	
private:
    enum ReceiverState
    {
        RS_WAITHEADER,
        RS_WAITDATA,
    } m_state;

    bool IsHeaderValid(CPacketHeader *header);
    uint32_t GetBodyLen(CPacketHeader *header);
    int GetHeader();
    int GetData();
};

#endif
