#include <cstdio>
#include <unistd.h>
#include "ttcapi.h"
#include "md5.h"

static const char *ttc_ip;
static const char *ttc_port;
static const char *dump_file;
static unsigned int step = 1000;

void usage(const char *app)
{
    printf("%s -i ttc_ip -p ttc_port -f dump_file [-s step]\n", app);
}

void parse_args(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "i:p:f:s:")) != -1)
    {
        switch(c)
        {
        case 'i':
            ttc_ip = optarg;
            break;
        case 'p':
            ttc_port = optarg;
            break;
        case 'f':
            dump_file = optarg;
            break;
        case 's':
            step = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind < argc || !ttc_ip || !ttc_port || !dump_file)
    {
        usage(argv[0]);
        exit(1);
    }
}

void MD5DigestHex(const unsigned char *msg, int len, unsigned char *digest) {
	struct MD5Context ctx;
	unsigned char h[16];
	const char xdigit[] = "0123456789abcdef";
	int i;

	MD5Init (&ctx);
	MD5Update (&ctx, msg, len);
	MD5Final (h, &ctx);
	for(i=0; i<16; i++) {
		*digest++ = xdigit[h[i]>>4];
		*digest++ = xdigit[h[i]&0xf];
	}
	*digest = '\0';
}

using namespace TTC;

int main(int argc, char **argv)
{
    parse_args(argc, argv);

    FILE *fdump = fopen(dump_file, "w");
    if(!fdump)
    {
        perror("fopen");
        return -1;
    }

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

            if(kl != 4)
            {
                printf("key length (%d) should be 4!!\n", kl);
                return -2;
            }

            char buf[64];
            MD5DigestHex((const unsigned char *)value, vl,
                    (unsigned char *)buf);
            fprintf(fdump, "%u\t%s\n", *(unsigned int *)key, buf);
        }

        start += step;
    }

    fclose(fdump);
    printf("dumped ok\n");
    return 0;
}

