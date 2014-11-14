#include <cstdio>
#include <cstring>
#include <string>

#include "ttcapi.h"

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        printf("usage: %s ttc_addr\n", argv[0]);
        return 1;
    }

    const char *addr = argv[1];

    TTC::Server server;
    server.SetAddress(addr);
    server.SetTableName("*");
    server.IntKey();

    TTC::SvrAdminRequest request(&server);
    request.SetAdminCode(TTC::GetClusterState);
    request.Need("value");

    TTC::Result r;
    if(request.Execute(r) != 0)
    {
        printf("error, %s %s\n", r.ErrorFrom(), r.ErrorMessage());
        return 1;
    }

    for(int i = 0; i < r.NumRows(); ++i)
    {
        if(r.FetchRow() < 0)
        {
            printf("fetch error, %s %s\n", r.ErrorFrom(), r.ErrorMessage());
            return 1;
        }

        int len = 0;
        const char *ptr = r.BinaryValue("value", &len);
        std::string tmp(ptr, len);
        printf("%s\n", tmp.c_str());
    }
    return 0;
}
