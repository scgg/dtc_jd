#include <cstdio>
#include <sys/types.h>
#include <sys/shm.h>

#include "raw_data.h"
#include "ttcapi.h"

#define PREV_INUSE 0x1
#define RESERVE_BITS (0x2|0x4)
#define SIZE_BITS (PREV_INUSE|RESERVE_BITS)

#define chunk2mem(h)   (void*)(((char*)h) + 2*sizeof(ALLOC_SIZE_T))
#define prev_inuse(p)       ((p)->m_tSize & PREV_INUSE)
#define CHUNK_SIZE(p)         ((p)->m_tSize & ~(SIZE_BITS))

CMemHead *memHeader;
CBinMalloc *mallocator;
int keySize;

int AttachShm(int shmKey)
{
    int id = shmget(shmKey, 0, 0);
    if(id == -1)
    {
        fprintf(stderr, "shmget %d error, %m", shmKey);
        return -1;
    }

    struct shmid_ds ds;
    if(shmctl(id, IPC_STAT, &ds) < 0)
    {
        fprintf(stderr, "shmctl %d error, %m", shmKey);
        return -1;
    }

    void *ptr = shmat(id, NULL, SHM_RDONLY);
    if(ptr == (void *)-1)
    {
        fprintf(stderr, "shmat %d error, %m", shmKey);
        return -1;
    }

    memHeader = (CMemHead *)ptr;
    mallocator = CBinMalloc::Instance();
    if(mallocator->Attach(ptr, memHeader->m_tSize) != 0)
    {
        fprintf(stderr, "bin malloc attach error\n");
        return -1;
    }

    return 0;
}

char *GetKeyByHandle(INTER_HANDLE_T handle, int *len)
{
    CMallocChunk *chunk = (CMallocChunk*)mallocator->Handle2Ptr(handle);
    CRawFormat *raw = (CRawFormat *)chunk2mem(mallocator->Handle2Ptr(handle));
    if ((raw->m_uchDataType != DATA_TYPE_RAW) 
            ||(raw->m_uiDataSize > CHUNK_SIZE(chunk))
            ||(raw->m_uiRowCnt > CHUNK_SIZE(chunk))
            ||(raw->m_uiRowCnt > raw->m_uiDataSize))
        return NULL;

    char *key = raw->m_achKey;
    if (key)
    {
        *len = keySize == 0 ? (*(unsigned char *)key + 1) : keySize;
    }
    return key;
}

int MoveChunk(INTER_HANDLE_T handle, TTC::Server *server)
{
    int len = 0;
    char *key = GetKeyByHandle(handle, &len);
    if(key == NULL)
    {
        fprintf(stderr, "get key from handle %llu failed\n",
                (unsigned long long)handle);
        return -1;
    }

    TTC::SvrAdminRequest request(server);
    request.SetAdminCode(TTC::NodeHandleChange);
    request.EQ("key", key, len);
    TTC::Result result;
    if(request.Execute(result) != 0)
    {
        fprintf(stderr, "move handle %llu failed, %s: %s\n",
                (unsigned long long)handle,
                result.ErrorFrom(), result.ErrorMessage());
        return -1;
    }

    return 0;
}

void LowerTop(int howmany, TTC::Server *server)
{
    int i = 0;
    INTER_HANDLE_T lastTop = 0;
    while(--howmany >= 0)
    {
        volatile CMemHead *header = (volatile CMemHead *)memHeader;
        INTER_HANDLE_T handle = header->m_hTop;

        if(lastTop == handle)
        {
            printf("top remain as the same, break now.\n");
            return;
        }

        CMallocChunk *chunk = (CMallocChunk *)mallocator->Handle2Ptr(handle);
        if(!prev_inuse(chunk))
        {
            fprintf(stderr, "the top chunk should have prev_inuse!\n");
            return;
        }
        printf("presize = %d\n", chunk->m_tPreSize);

        lastTop = handle;
        handle -= chunk->m_tPreSize;
        if(MoveChunk(handle, server) != 0)
            break;
    }
}

void usage(const char *app)
{
    printf("%s -i ttc_ip -p ttc_port -k shmkey -n nodes -s keysize\n", app);
}

int nodes = 100;
const char *ttc_ip;
const char *ttc_port;
int shmkey;

void parse_args(int argc, char **argv)
{
    int c = 0;
    while((c = getopt(argc, argv, "i:p:k:n:s:")) != -1)
    {
        switch(c)
        {
        case 'i':
            ttc_ip = optarg;
            break;
        case 'p':
            ttc_port = optarg;
            break;
        case 'k':
            shmkey = atoi(optarg);
            break;
        case 'n':
            nodes = atoi(optarg);
            break;
        case 's':
            keySize = atoi(optarg);
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(optind < argc || !ttc_ip || !ttc_port || !shmkey)
    {
        usage(argv[0]);
        exit(1);
    }
}


int main(int argc, char **argv)
{
    parse_args(argc, argv);

    TTC::Server server;
    server.StringKey();
    server.SetAddress(ttc_ip, ttc_port);
    server.SetTableName("*");

    if(AttachShm(shmkey) != 0)
    {
        fprintf(stderr, "attach shm error, exit now.\n");
        return -1;
    }

    LowerTop(nodes, &server);
    return 0;
}



