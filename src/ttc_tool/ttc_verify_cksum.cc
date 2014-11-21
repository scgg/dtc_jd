#include <cstdio>
#include <unistd.h>
#include <cstring>
#include "ttcapi.h"
#include "md5.h"

static const char *ttc_ip;
static const char *ttc_port;
static const char *dump_file;

void usage(const char *app)
{
    printf("%s -i ttc_ip -p ttc_port -f dump_file \n", app);
}

void parse_args(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "i:p:f:")) != -1)
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

void MD5DigestHex( const unsigned char *msg, int len, unsigned char *digest) {
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

void TTCRegisterHB(TTC::Server *server)
{
    TTC::SvrAdminRequest req(server);
    req.SetAdminCode(TTC::RegisterHB);
    req.SetHotBackupID(0);
    TTC::Result res;
    int ret = req.Execute(res);
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);

    FILE *fdump = fopen(dump_file, "r");
    if(!fdump)
    {
        perror("fopen");
        return -1;
    }

    Server ttcServer;
    ttcServer.StringKey();
    ttcServer.SetAddress(ttc_ip, ttc_port);
    ttcServer.SetTableName("*");

    TTCRegisterHB(&ttcServer);

    char *line = NULL;
    size_t len = 0;

    while(getline(&line, &len, fdump) != -1)
    {
        char *space = strchr(line, '\t');
        if(!space)
            continue;

        *space = 0;
        space++;
        unsigned int key = strtoul(line, NULL, 10);
        SvrAdminRequest request(&ttcServer);
        request.SetAdminCode(TTC::GetRawData);
        request.EQ("key", (const char *)&key, (int)sizeof(key));
        request.Need("type");
        request.Need("flag");
        request.Need("value");

        TTC::Result res;
        int ret = request.Execute(res);
        if(ret != 0)
        {
            printf("%12u GetRawData error, %s : %s\n",
                    key, res.ErrorFrom(), res.ErrorMessage());
            continue;
        }

        if(res.NumRows() <= 0)
        {
            printf("%12u GetRawData num rows %d <= 0\n",
                    key, res.NumRows());
            continue;
        }

        res.FetchRow();
        int type = res.IntValue("type");
        int flag = res.IntValue("flag");
        if(flag == 8)
        {
            printf("%12u does not exist in ttc\n", key);
            continue;
        }

        int l = 0;
        const char *ptr = res.BinaryValue("value", &l);
        unsigned char digest[64];
        MD5DigestHex((const unsigned char *)ptr, l, digest);
        if(memcmp(space, digest, 16*2) != 0)
        {
            printf("%12u cksum mismatch!\n", key);
        }
    }

    fclose(fdump);
    return 0;
}

