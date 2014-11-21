#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

#include "ttcapi.h"

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        printf("usage: %s ttc_addr conf_file\n", argv[0]);
        return 1;
    }

    const char *addr = argv[1];
    int fd = open(argv[2], O_RDONLY);
    if(fd < 0)
    {
        printf("open %s error, %m\n", argv[2]);
        return 1;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if(size == (off_t) -1)
    {
        printf("lseek error, %m\n");
        return 1;
    }
    lseek(fd, 0, SEEK_SET);

    char *buf = (char *)malloc(size);
    assert(read(fd, buf, size) == size);
    close(fd);

    TTC::Server server;
    server.SetAddress(addr);
    server.SetTableName("*");
    server.IntKey();

    TTC::SvrAdminRequest request(&server);
    request.SetAdminCode(TTC::ReloadClusterNodeList);
    request.Set("value", buf, size);

    TTC::Result r;
    if(request.Execute(r) != 0)
    {
        printf("error, %s %s\n", r.ErrorFrom(), r.ErrorMessage());
        return 1;
    }

    printf("succeed.\n");
    return 0;
}
