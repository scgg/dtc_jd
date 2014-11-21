#include <cstdio>
#include <cstring>

#include "ttcapi.h"

enum MigrationState
{
    MS_NOT_STARTED,
    MS_MIGRATING,
    MS_MIGRATE_READONLY,
    MS_COMPLETED,
};

const char *state_str[] = {
    "not_started",
    "migrating",
    "migration_readonly",
    "completed",
};

int main(int argc, char **argv)
{
    if(argc != 4)
    {
        printf("usage: %s ttc_addr node_name [not_started|migrating|migration_readonly|completed]\n", argv[0]);
        return 1;
    }

    const char *addr = argv[1];
    const char *name = argv[2];
    int state = 0;

    for(state = MS_NOT_STARTED; state <= MS_COMPLETED; ++state)
    {
        if(strcmp(argv[3], state_str[state]) == 0)
        {
            printf("put node %s to %s state : ", name, state_str[state]);
            break;
        }
    }

    if(state > MS_COMPLETED)
    {
        printf("unknown state %s\n", argv[3]);
        return 1;
    }

    TTC::Server server;
    server.SetAddress(addr);
    server.SetTableName("*");
    server.IntKey();

    TTC::SvrAdminRequest request(&server);
    request.SetAdminCode(TTC::SetClusterNodeState);
    request.Set("key", name);
    request.Set("flag", state);

    TTC::Result r;
    if(request.Execute(r) != 0)
    {
        printf("error, %s %s\n", r.ErrorFrom(), r.ErrorMessage());
        return 1;
    }

    printf("succeed.\n");
    return 0;
}
