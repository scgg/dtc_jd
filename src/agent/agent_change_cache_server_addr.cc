#include <cstdio>
#include <cstdlib>

#include "sockaddr.h"
#include "admin.h"
#include "constant.h"
#include "admin_protocol.pb.h"

int send_n(int fd, char *buf, int len)
{
    int sent = 0;
    while(sent < len)
    {
        int rtn = send(fd, buf + sent, len - sent, 0);
        if(rtn <= 0)
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
        if(rtn < 0)
            return -1;
        if(rtn == 0)
            return 0;

        recved += rtn;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if(argc != 5)
    {
        fprintf(stderr, "usage: %s admin_addr module name address\n", argv[0]);
        return 1;
    }

    CSocketAddress adminAddr;
    if(const char *errMsg = adminAddr.SetAddress(argv[1]))
    {
        fprintf(stderr, "invalid admin address %s: %s\n", argv[1], errMsg);
        return -1;
    }

    if(strlen(argv[3]) > 255)
    {
        fprintf(stderr, "invalid cache server name %s: length exceeds 255\n", argv[3]);
        return -1;
    }

    CSocketAddress cacheAddress;
    if(const char *errMsg = cacheAddress.SetAddress(argv[4]))
    {
        fprintf(stderr, "invalid cache server address %s: %s\n", argv[4], errMsg);
        return -1;
    }

    int module = atoi(argv[2]);

    int s = adminAddr.CreateSocket();
    if(s < 0)
    {
        perror("socket error");
        return -1;
    }

    struct timeval tv = { 1, 0 };
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if(adminAddr.ConnectSocket(s) < 0)
    {
        perror("connect error");
        return -1;
    }

    ttc::agent::ChangeCacheServerAddrRequest request;
    request.set_module(module);
    request.set_name(argv[3]);
    request.set_addr(argv[4]);

    int byteSize = request.ByteSize();
    unsigned char *buf = (unsigned char *)malloc(byteSize + sizeof(ProtocolHeader));
    ProtocolHeader *header = (ProtocolHeader *)buf;
    header->magic = ADMIN_PROTOCOL_MAGIC;
    header->length = byteSize;
    header->cmd = AC_ReplaceCacheServer;
    request.SerializeWithCachedSizesToArray(buf + sizeof(ProtocolHeader));

    if(send_n(s, (char *)buf, byteSize + sizeof(ProtocolHeader)) < 0)
    {
        perror("send error");
        return -1;
    }

    char response[65536];
    if(recv_n(s, response, sizeof(ProtocolHeader)) < 0)
    {
        perror("recv error");
        return -1;
    }

    header = (ProtocolHeader *)response;
    if(header->magic != ADMIN_PROTOCOL_MAGIC || header->cmd != AC_Reply)
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
        perror("recv error");
        return -1;
    }

    ttc::agent::Reply reply;
    if(!reply.ParseFromArray(
            (unsigned char *)response + sizeof(ProtocolHeader),
            header->length))
    {
        printf("parse reply error.\n");
        return -1;
    }


    if(reply.status() == AdminOk)
    {
        printf("cache server %s's address changed to %s.\n",
                argv[3], argv[4]);
        return 0;
    }

    fprintf(stderr, "change server address failed: %s\n", reply.msg().c_str());
    return 1;
}

