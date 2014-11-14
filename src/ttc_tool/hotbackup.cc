#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <stdint.h>
#include <set>
#include <vector>

#include "ttcapi.h"

static const char *master_ttc_ip;
static const char *master_ttc_port;
static const char *slave_ttc_ip;
static const char *slave_ttc_port;

static uint32_t serial = 0;
static uint32_t offset = 0;
static uint32_t start = 0;
static uint32_t step = 2000;

void usage(char *app)
{
    printf("%s -f master_ttc_ip -p master_ttc_port -t slave_ttc_ip -q slave_ttc_port"
            " -l start_from -s step -j serial -o offset\n", app);
}

void parse_arg(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "f:p:t:q:l:s:j:o:")) != -1)
    {
        switch(c)
        {
        case 'f':
            master_ttc_ip = optarg;
            break;
        case 't':
            slave_ttc_ip = optarg;
            break;
        case 'p':
            master_ttc_port = optarg;
            break;
        case 'q':
            slave_ttc_port = optarg;
            break;
        case 'l':
            start = strtoul(optarg, NULL, 10);
            break;
        case 'j':
            serial = strtoul(optarg, NULL, 10);
            break;
        case 'o':
            offset = strtoul(optarg, NULL, 10);
            break;
        case 's':
            step = strtoul(optarg, NULL, 10);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }


    if(optind < argc || !master_ttc_ip || !master_ttc_port || !slave_ttc_ip || !slave_ttc_port)
    {
        usage(argv[0]);
        exit(1);
    }
}

int TTCGetRawData(TTC::Server &s, const char *key, int kl,
        char **result, int *len)
{
    TTC::SvrAdminRequest req(&s);

    req.SetAdminCode(TTC::GetRawData);
    req.EQ("key", key, kl);
    req.Need("value");

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        printf("GetRawData error %s, %s\n",
                res.ErrorFrom(), res.ErrorMessage());
        return -1;
    }

    if(res.NumRows() <= 0)
        return -1;

    res.FetchRow();

    int l;
    const char *ptr = res.BinaryValue("value", &l);
    if(l > *len)
    {
        *result = (char *)realloc(*result, l);
        *len = l;
    }

    memcpy(*result, ptr, l);
    return 0;
}

int TTCReplaceRawData(TTC::Server &s,
        const char *key, int kl, 
        const char *value, size_t vl)
{
    TTC::SvrAdminRequest req(&s);
    req.SetAdminCode(TTC::ReplaceRawData);
    req.EQ("key", key, kl);
    req.Set("value", value, vl);

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        printf("replace to target failed: %s, %s\n",
                res.ErrorFrom(), res.ErrorMessage());
        return ret;
    }

    return 0;
}

int TTCRegisterHB(TTC::Server &server, uint32_t s, uint32_t o)
{
    uint64_t journal_id = s;
    journal_id = (journal_id << 32) | o;
    TTC::SvrAdminRequest req(&server);
    req.SetHotBackupID(journal_id);
    TTC::Result res;
    req.Execute(res);
    return res.ResultCode();
}

void DoFullSync(TTC::Server &master, TTC::Server &slave,
        uint32_t start, uint32_t step)
{
    int count = 0;
    while(true)
    {
        if(count % 1000 == 0)
            printf("get [%u %u] ...\n", start, step);

        TTC::SvrAdminRequest req_get_key_list(&master);
        req_get_key_list.SetAdminCode(TTC::GetKeyList);
        req_get_key_list.Need("key");
        req_get_key_list.Need("value");

        uint32_t limit = (unsigned int)~0UL - start;
        if(limit > step)
            limit = step;
        if(limit == 0)
            limit = 1;
        req_get_key_list.Limit(start, limit);

        TTC::Result res;
        int ret = req_get_key_list.Execute(res);
        if(ret == -TTC::EC_FULL_SYNC_COMPLETE)
        {
            printf("full sync done!!!!!!!!!!\n");
            return;
        }

        if(ret != 0)
        {
            printf("get key list [%u, %u] failed, %s : %s\n",
                    start, limit, res.ErrorFrom(), res.ErrorMessage());
            master.Close();
            master.Connect();
            sleep(1);
            continue;
        }

        for(int i = 0; i < res.NumRows(); ++i)
        {
            if(res.FetchRow() != 0)
            {
                printf("fetch row error: %s\n", res.ErrorMessage());
                exit(-2);
            }

            int kl, vl;
            const char *key = res.BinaryValue("key", &kl);
            const char *value = res.BinaryValue("value", &vl);
            
            if(TTCReplaceRawData(slave, key, kl, value, vl) != 0)
            {
                sleep(1);
                slave.Close();
                slave.Ping();
                goto retry;
            }
        }

        start += limit;
retry:
        ;
    }
}

void DoIncSync(TTC::Server &master, TTC::Server &slave)
{
    uint64_t journal_id = serial;
    journal_id = (journal_id << 32) | offset;
    char *value = NULL;
    int vl = 0;

    int i = 0;
    while(true)
    {
        TTC::SvrAdminRequest req(&master);
        req.SetAdminCode(TTC::GetUpdateKey);
        req.SetHotBackupID(journal_id);
        req.Limit(0, step);
        req.Need("key");
        req.Need("type");

        TTC::Result res;
        int ret = req.Execute(res);
        if(ret != 0)
        {
            printf("GetUpdateKey [%u %u] error, %s: %s\n",
                    (uint32_t)(journal_id >> 32),
                    (uint32_t)journal_id,
                    res.ErrorFrom(), res.ErrorMessage());
            sleep(1);
            continue;
        }

        printf("GetUpdateKey [%u %u] got %d keys\n",
                (uint32_t)(journal_id >> 32),
                (uint32_t)journal_id,
                res.NumRows());

        std::set<std::vector<char> > ids;
        for(int i = 0; i < res.NumRows(); ++i)
        {
            res.FetchRow();
            int kl;
            const char *key = res.BinaryValue("key", &kl);
            ids.insert(std::vector<char>(key, key + kl));
        }

        for(std::set<std::vector<char> >::iterator i = ids.begin();
                i != ids.end(); ++i)
        {
            if(TTCGetRawData(master, &(i->front()), i->size(), &value, &vl) != 0)
            {
                exit(-2);
            }

            if(TTCReplaceRawData(slave, &(i->front()), i->size(), value, vl) != 0)
            {
                exit(-3);
            }
        }

        journal_id = res.HotBackupID();
    }
}

int main(int argc, char **argv)
{
    parse_arg(argc, argv);

    setlinebuf(stdout);

    TTC::Server master, slave;
    master.StringKey();
    master.SetTableName("*");
    master.SetAddress(master_ttc_ip, master_ttc_port);
    master.Ping();

    switch(TTCRegisterHB(master, serial, offset))
    {
    case -TTC::EC_INC_SYNC_STAGE:
        printf("joural id accepted.\n");
        break;
    case -TTC::EC_FULL_SYNC_STAGE:
        if(start != 0)
        {
            printf("start should be 0 if you want to do full sync.\n");
            return 2;
        }
        break;
    case -TTC::EC_ERR_SYNC_STAGE:
        printf("joural id rejected by server. restart hotbackup please.\n");
        return 3;
    }

    slave.StringKey();
    slave.SetTableName("*");
    slave.SetAddress(slave_ttc_ip, slave_ttc_port);
    slave.SetTimeout(30);
    slave.Ping();

    DoFullSync(master, slave, start, step);
    DoIncSync(master, slave);

    return 0;
}


