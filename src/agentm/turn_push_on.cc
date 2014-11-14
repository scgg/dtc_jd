#include <cstdio>
#include <cstdlib>

#include "sockaddr.h"
#include "admin.h"
#include "admin_protocol.pb.h"

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

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        fprintf(stderr, "usage: %s agetnm_ip:port on|off\n", argv[0]);
        return 1;
    }

    CSocketAddress agtmAddr;
    if(const char *errMsg = agtmAddr.SetAddress(argv[1]))
    {
        fprintf(stderr, "invalid agentm address %s: %s\n", argv[1], errMsg);
        fprintf(stderr, "usage: %s agetnm_ip port on|off\n", argv[0]);
        return -1;
    }

    bool push = false;
    if( 0 == strcmp("on",argv[2]))
    {
        push = true;
    }
    else if( 0 == strcmp("off", argv[2]))
    {
        push = false;
    }
    else
    {
        fprintf(stderr, "usage: %s agetnm_ip port on|off\n", argv[0]);
        return 1;
    }

    int s = agtmAddr.CreateSocket();
    if(s < 0)
    {
        perror("socket error");
        return -1;
    }

    struct timeval tv = { 1, 0 };
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if(agtmAddr.ConnectSocket(s) < 0)
    {
        perror("connect error");
        return -1;
    }
    else
    {
        printf("connected.\n");
    }

    ttc::agent::SetPush request;
    if(push)
        request.set_push(1);
    else
        request.set_push(0);

    int byteSize = request.ByteSize();
    unsigned char *buf = (unsigned char *)malloc(byteSize + sizeof(ProtocolHeader));
    ProtocolHeader *header = (ProtocolHeader *)buf;
    header->magic = ADMIN_PROTOCOL_MAGIC;
    header->length = byteSize;
    header->cmd = AC_SetPush;
    request.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));

    if(send_n(s, (char *)buf, byteSize + sizeof(ProtocolHeader)) < 0)
    {
        printf("send error\n");
        return -1;
    }

    printf("sent[%u].\n", byteSize  + sizeof(ProtocolHeader) );

    char response[65536];
    //first get the header
    if(recv_n(s, response, sizeof(ProtocolHeader)) < 0)
    {
        printf("recv header error\n");
        return -1;
    }

    header = (ProtocolHeader *)response;
    if(header->magic != ADMIN_PROTOCOL_MAGIC || header->cmd != AC_SetPushReply)
    {
        printf("invalid header received: magic %x, cmd %d\n",
                header->magic, header->cmd);
        return -1;
    }

    if(header->length > sizeof(response) - sizeof(ProtocolHeader))
    {
        printf("packet too big: %d\n", header->length);
        return -1;
    }

    if(recv_n(s, response + sizeof(ProtocolHeader), header->length) < 0)
    {
        printf("recv body error\n");
        return -1;
    }

    ttc::agent::SetPushReply reply;
    if(!reply.ParseFromArray(
            (unsigned char *)response + sizeof(ProtocolHeader),
            header->length))
    {
        printf("parse reply error.\n");
        return -1;
    }

    if(reply.ok() == 1)
    {
        printf("set push done.\n");
        return 0;
    }

    printf("set push failed.\n");
    return 1;
}
