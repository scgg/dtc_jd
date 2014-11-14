#include <stdint.h>
#include <errno.h>

#include <stdio.h>

#include <new>

#include "packet.h"
#include "protocol.h"

int CPacket::EncodeHeader( CPacketHeader &header)
{
	int len = sizeof(header);
	for(int i=0; i<DRequest::Section::Total; i++) {
		len += header.len[i];

	}
	return len;
}


int CPacket::Send(int fd)
{
	if(nv <= 0) return SendResultDone;

	int rv;
	while(true)//���Է������ݡ�  
	{   
		//ResetEvent(pClient->m_hEvent);  
		//���óɷ�����socket
	//	unsigned long ul=1;  
	//	ioctlsocket(fd,FIONBIO,&ul); 
		int rv=send(fd,(char*)v->iov_base,v->iov_len,0);  
		if(rv<0)	//����ʧ��  
		{  
			int r=WSAGetLastError();  
			if(r==WSAEWOULDBLOCK)		//�׽��ֻ�������û������
			{  
				continue;  
			}
			if(errno==EINTR || errno==EAGAIN || errno==EINPROGRESS)
				return SendResultMoreData;  
		}  
		else   //���ͳɹ�
		{
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
	}  

}
