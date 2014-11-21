#include <cstdio>
#include <unistd.h>
#include <stdint.h>

#include "ttcapi.h"

static const char *ttc_ip;
static const char *ttc_port;
static unsigned int step = 1000;

void usage(const char *app)
{
    printf("%s -i ttc_ip -p ttc_port [-s step]\n", app);
}

void parse_args(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "i:p:s:")) != -1)
    {
        switch(c)
        {
        case 'i':
            ttc_ip = optarg;
            break;
        case 'p':
            ttc_port = optarg;
            break;
        case 's':
            step = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind < argc || !ttc_ip || !ttc_port)
    {
        usage(argv[0]);
        exit(1);
    }
}

using namespace TTC;

void PurgeIfEmpty(TTC::Server &s, const char *key, int len)
{
    TTC::SvrAdminRequest req(&s);
    req.SetAdminCode(TTC::GetRawData);
    req.EQ("key", key, len);
    req.Need("type");
    req.Need("flag");
    req.Need("value");

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        printf("GetRawData error, %s : %s\n",
                res.ErrorFrom(), res.ErrorMessage());
        return;
    }

    if(res.NumRows() <= 0)
    {
        printf("GetRawData num rows %d <= 0\n", res.NumRows());
        return;
    }

    res.FetchRow();
    int type = res.IntValue("type");
    int flag = res.IntValue("flag");

}

struct CRawFormat{
    unsigned char   m_uchDataType;  // 数据类型CDataType
    uint32_t m_uiDataSize;          // 数据总大小
    uint32_t m_uiRowCnt;            // 行数
    char    m_achKey[0];            // key
    char    m_achRows[0];           // 行数据
}__attribute__((packed));

int GetRowNumberFromRaw(const char *value, int len)
{
    const CRawFormat *raw = (const CRawFormat *)value;
    if(raw->m_uchDataType != 0 || raw->m_uiDataSize != len)
    {
        printf("raw data decode error, type: %d, size %u, len %d\n",
                raw->m_uchDataType, raw->m_uiDataSize, len);
        return -1;
    }

    return raw->m_uiRowCnt;
}

void PurgeEmptyNode(TTC::Server &s, const char *key, int len)
{
    TTC::SvrAdminRequest req(&s);
    req.SetAdminCode(TTC::ReplaceRawData);
    req.EQ("key", key, len);
    req.Set("type", 0);
    req.Set("flag", 4); //empty node
    req.Set("value", (char *)NULL, 0);

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        printf("replace raw data error: %s, %s\n",
                res.ErrorFrom(), res.ErrorMessage());
    }
}


int main(int argc, char **argv)
{
    parse_args(argc, argv);

    Server ttcServer;
    ttcServer.StringKey();
    ttcServer.SetAddress(ttc_ip, ttc_port);
    ttcServer.SetTableName("*");

    unsigned int start = 0;

    while(true)
    {
        SvrAdminRequest request(&ttcServer);
        request.SetAdminCode(GetKeyList);
        request.Need("key");
        request.Need("value");
        request.Limit(start, step);

        Result result;
        int ret = request.Execute(result);
        if(ret == -EC_FULL_SYNC_COMPLETE)
            break;
        else if(ret != 0)
        {
            printf("get range [%u %u] failed: %s, %s\n",
                    start, start + step,
                    result.ErrorFrom(), result.ErrorMessage());
            break;
        }

        for(int i = 0; i < result.NumRows(); ++i)
        {
            if(result.FetchRow() != 0)
            {
                printf("fetch row error: %s\n", result.ErrorMessage());
                continue;
            }

            const char *key, *value;
            int kl, vl;
            key = result.BinaryValue("key", &kl);
            value = result.BinaryValue("value", &vl);

            if(GetRowNumberFromRaw(value, vl) == 0)
            {
                PurgeEmptyNode(ttcServer, key, kl);
            }
        }

        start += step;
    }

    return 0;
}

