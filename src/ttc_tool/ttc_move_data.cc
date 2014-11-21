#include <cstdio>
#include <unistd.h>
#include <cstring>

#include "ttcapi.h"

static const char *from_ttc_ip;
static const char *from_ttc_port;
static const char *to_ttc_ip;
static const char *to_ttc_port;
static const char *key_file;
static bool force = false;

void usage(char *app)
{
    printf("%s -i from_ttc_ip -p from_ttc_port -j to_ttc_ip -q to_ttc_port -k key_file [-f]\n"
            " -f replace data even if the key exists in target ttc\n", app);
}

void parse_arg(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "i:p:j:q:k:f")) != -1)
    {
        switch(c)
        {
        case 'i':
            from_ttc_ip = optarg;
            break;
        case 'j':
            to_ttc_ip = optarg;
            break;
        case 'p':
            from_ttc_port = optarg;
            break;
        case 'q':
            to_ttc_port = optarg;
            break;
        case 'f':
            force = true;
            break;
        case 'k':
            key_file = optarg;
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }


    if(optind < argc || !from_ttc_ip || !from_ttc_port || !to_ttc_ip || !to_ttc_port || !key_file)
    {
        usage(argv[0]);
        exit(1);
    }
}

int TTCGetRawData(TTC::Server &s, unsigned int key, char **result, size_t *len, 
        unsigned int *type, unsigned int *flag, bool noisy)
{
    TTC::SvrAdminRequest req(&s);

    req.SetAdminCode(TTC::GetRawData);
    req.EQ("key", (const char *)&key, (int)sizeof(unsigned int));
    req.Need("type");
    req.Need("flag");
    req.Need("value");

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        if(noisy)
            printf("%12u: GetRawData error %s, %s\n",
                key, res.ErrorFrom(), res.ErrorMessage());
        return -1;
    }

    if(res.NumRows() <= 0)
        return -1;

    if(res.FetchRow() != 0)
    {
        printf("FetchRow error, %s: %s\n", res.ErrorFrom(), res.ErrorMessage());
        return -1;
    }
    *type = res.IntValue("type");
    *flag = res.IntValue("flag");
    if(*flag == 8)
    {
        if(noisy)
            printf("%12u: key not exist\n", key);
        return -1;
    }

    int l;
    const char *ptr = res.BinaryValue("value", &l);
    if(l > *len)
    {
        *result = (char *)realloc(*result, l);
        *len = l;
    }
    printf("type = %d, flag = %d, value len = %d\n", *type, *flag, l);

    memcpy(*result, ptr, l);
    return 0;
}

int TTCReplaceRawData(TTC::Server &s, unsigned int key, 
        unsigned int type, unsigned int flag, char *ptr, size_t len)
{
    TTC::SvrAdminRequest req(&s);
    req.SetAdminCode(TTC::ReplaceRawData);
    req.EQ("key", (const char *)&key, (int)sizeof(key));
    req.Set("type", type);
    req.Set("flag", flag);
    req.Set("value", ptr, len);

    TTC::Result res;
    int ret = req.Execute(res);
    if(ret != 0)
    {
        printf("%12u replace to target failed: %s, %s\n",
                key, res.ErrorFrom(), res.ErrorMessage());
        return ret;
    }

    return 0;
}

int main(int argc, char **argv)
{
    parse_arg(argc, argv);

    FILE *kf = fopen(key_file, "r");
    if(!kf)
    {
        perror("open key file error");
        return -1;
    }

    TTC::Server fromTTC, toTTC;
    fromTTC.StringKey();
    fromTTC.SetTableName("*");
    fromTTC.SetAddress(from_ttc_ip, from_ttc_port);

    toTTC.StringKey();
    toTTC.SetTableName("*");
    toTTC.SetAddress(to_ttc_ip, to_ttc_port);

    char *line = NULL;
    size_t len = 0;
    char *result_from = NULL, *result_to = NULL;
    size_t len_from = 0, len_to = 0;
    unsigned type_from, type_to;
    unsigned flag_from, flag_to;
    while(getline(&line, &len, kf) != -1)
    {
        unsigned int key = strtoul(line, NULL, 10);
        if(TTCGetRawData(fromTTC, key, &result_from, &len_from, &type_from, &flag_from, true) != 0)
            continue;
        printf("get raw data from source ttc, len %d\n", len_from);
        
        if(TTCGetRawData(toTTC, key, &result_to, &len_to, &type_to, &flag_to, false) == 0)
        {
            printf("%12u exists in target ttc, %s\n", key, force ? "replace !" : "will not replace.");
            if(!force)
                continue;
        }

        TTCReplaceRawData(toTTC, key, type_from, flag_from, result_from, len_from);
    }
}


