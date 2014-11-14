#include <cstdio>
#include <cstring>

#include "ttcapi.h"

int main(int argc, char **argv)
{
    if(argc != 4)
    {
        printf("usage: %s ttc_addr node_name node_addr\n", argv[0]);
        return 1;
    }

    const char *addr = argv[1];
    const char *name = argv[2];
    const char *node_addr = argv[3];

    TTC::Server server;
    server.SetAddress(addr);
    server.SetTableName("*");
    server.IntKey();

    TTC::SvrAdminRequest request(&server);
    request.SetAdminCode(TTC::ChangeNodeAddress);
    request.Set("key", name);
    request.Set("value", node_addr);

    TTC::Result r;
    if(request.Execute(r) != 0)
    {
        printf("error, %s %s\n", r.ErrorFrom(), r.ErrorMessage());
        return 1;
    }

    printf("succeed.\n");
    return 0;
}
