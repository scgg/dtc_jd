/*
 * =====================================================================================
 *
 *       Filename:  receiver.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/31/2010 12:54:13 AM
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
#include <string.h>

#include "receiver.h"
#include "log.h"

CAGReceiver::CAGReceiver(int f):
    m_buf(NULL),
    m_bufSize(0),
    m_offset(0),
    m_fd(f), m_state(RS_WAITHEADER)
{
}

CAGReceiver::~CAGReceiver()
{
    if(m_buf)
        free(m_buf);
}


int CAGReceiver::Recv()
{
    int rtn = 0;
    switch(m_state)
    {
    case RS_WAITHEADER:
        rtn = GetHeader();
        if(rtn < 0)
            return -1;
        if(rtn == 0)
            return 0;
    case RS_WAITDATA:
        return GetData();

    default:
        return -1;
    }
}

int CAGReceiver::GetHeader()
{
    int recved = recv(m_fd, m_header + m_offset, sizeof(CPacketHeader) - m_offset, 0);
    if(recved < 0)
    {
        if(EAGAIN == errno || EINTR == errno || EINPROGRESS == errno)
            return 0;
        log_error("receiver recv error fd[%d] : %m", m_fd);
        return -1;
    }

    if(recved == 0)
    {
        log_debug("remote close fd[%d]", m_fd);
        return -1;
    }
    m_offset += recved;

    if(m_offset == sizeof(CPacketHeader))
    {
        CPacketHeader *header = (CPacketHeader *)m_header;
        if(!IsHeaderValid(header))
        {
            log_error("invalid header received");
            return -1;
        }

        m_bufSize = GetBodyLen(header) + sizeof(CPacketHeader);
        m_buf = (char *)malloc(m_bufSize);
        memcpy(m_buf, m_header, sizeof(CPacketHeader));
        m_state = RS_WAITDATA;

        return 1;
    }

    return 0;
}

bool CAGReceiver::IsHeaderValid(CPacketHeader *header)
{
    if(GetBodyLen(header) > MAXPACKETSIZE)
    {
    	log_debug("len of header is :%u",GetBodyLen(header));
        return false;
    }
    return true;
}

uint32_t CAGReceiver::GetBodyLen(CPacketHeader *header)
{
    int len = 0;
    for(int i = 0; i < DRequest::Section::Total; ++i)
    {
        len += header->len[i];
    }

    return len;
}

int CAGReceiver::GetData()
{
    int recved = recv(m_fd, m_buf + m_offset, m_bufSize - m_offset, 0);
    if(recved < 0)
    {
        if(EAGAIN == errno || EINTR == errno || EINPROGRESS == errno)
            return 0;
        log_error("receiver recv error fd[%d] : %m", m_fd);
        return -1;
    }

    if(recved == 0)
    {
        log_debug("remote close fd[%d]", m_fd);
        return -1;
    }
    m_offset += recved;

    return m_offset == m_bufSize ? 1 : 0;
}

void CAGReceiver::GetPacket(char **buf, uint32_t *len)
{
    *len = m_bufSize;
    *buf = m_buf;
    m_buf = 0;
    m_bufSize = 0;
    m_offset = 0;
    m_state = RS_WAITHEADER;
}



