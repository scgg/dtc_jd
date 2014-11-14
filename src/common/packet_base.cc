#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <new>

#include "packet.h"
#include "protocol.h"

#include "log.h"
int CPacket::Send(int fd)
{
	if(nv <= 0) return SendResultDone;
	
	//int rv = writev(fd, v, nv);
	struct msghdr msgh;
	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;
	msgh.msg_iov = v;
	msgh.msg_iovlen = nv;
	msgh.msg_control = NULL;
	msgh.msg_controllen = 0;
	msgh.msg_flags = 0;

	int rv = sendmsg(fd, &msgh, MSG_DONTWAIT|MSG_NOSIGNAL);
	
	
	if(rv < 0) {
	    if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
		return SendResultMoreData;
	    return SendResultError;
	}
	if(rv==0)
	    return SendResultMoreData;
	bytes -= rv;
	if(bytes==0) {
	    nv = 0;
	    return SendResultDone;
	}
	while(nv>0 && rv >= (int64_t)v->iov_len) {
	    rv -= v->iov_len;
	    v++;
	    nv--;
	}
	if(rv > 0) {
	    v->iov_base = (char *)v->iov_base + rv;
	    v->iov_len -= rv;
	}
	return nv==0 ? SendResultDone : SendResultMoreData;
}

int CPacket::SendTo(int fd, void *addr, int len)
{
	if(nv <= 0) return SendResultDone;
	struct msghdr msgh;
	msgh.msg_name = addr;
	msgh.msg_namelen = len;
	msgh.msg_iov = v;
	msgh.msg_iovlen = nv;
	msgh.msg_control = NULL;
	msgh.msg_controllen = 0;
	msgh.msg_flags = 0;

	int rv = sendmsg(fd, &msgh, MSG_DONTWAIT|MSG_NOSIGNAL);

	if(rv < 0) {
#if 0
	    if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
		return SendResultMoreData;
#endif
	    return SendResultError;
	}
	if(rv==0)
	    return SendResultError;

	bytes -= rv;
	if(bytes==0) {
	    nv = 0;
	    return SendResultDone;
	}
	return SendResultError;
}

int CPacket::EncodeHeader( CPacketHeader &header)
{
	int len = sizeof(header);
	for(int i=0; i<DRequest::Section::Total; i++) {
		len += header.len[i];
#if __BYTE_ORDER == __BIG_ENDIAN
		header.len[i] = bswap_32(header.len[i]);
#endif
	}
	return len;
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
int CPacket::EncodeHeader(const CPacketHeader &header)
{
	int len = sizeof(header);
	for(int i=0; i<DRequest::Section::Total; i++) {
		len += header.len[i];
	}
	return len;
}
#endif

