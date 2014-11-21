#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <cstring>
#include <cassert>
#include <cstdlib>

#include "new_hash.h"
#include "log.h"
#include "file_backed_key_set.h"

CFileBackedKeySet::hash_node_allocator::~hash_node_allocator()
{
    reset();
}

void CFileBackedKeySet::hash_node_allocator::reset()
{
    for(std::vector<char *>::iterator iter = m_buffs.begin();
            iter != m_buffs.end(); ++iter)
    {
        ::free(*iter);
    }
    m_buffs.clear();
    m_freeNode = 0;
}

CFileBackedKeySet::hash_node *CFileBackedKeySet::hash_node_allocator::alloc()
{
    if(!m_freeNode)
    {
        CFileBackedKeySet::hash_node *nodes = (hash_node *)calloc(GROW_COUNT, sizeof(hash_node));

        CFileBackedKeySet::hash_node *n = nodes;
        for(int i = 0; i < GROW_COUNT - 1; ++i)
        {
            n->next = n + 1;
            ++n;
        }
        m_freeNode = nodes;
        m_buffs.push_back((char *)nodes);
    }

    hash_node *rtn = m_freeNode;
    m_freeNode = (hash_node *)m_freeNode->next;
    return rtn;
}

void CFileBackedKeySet::hash_node_allocator::free(hash_node *n)
{
    n->next = m_freeNode;
    m_freeNode = n;
}


CFileBackedKeySet::CFileBackedKeySet(const char *file, int keySize):
    m_filePath(file), m_fd(-1), m_keySize(keySize), m_base((char *)MAP_FAILED),
    m_buckets(0)
{
}

CFileBackedKeySet::~CFileBackedKeySet()
{
    if(m_fd >= 0)
    {
        if(m_base != MAP_FAILED)
            munmap(m_base, get_meta_info()->size);
        close(m_fd);
    }

    if(m_buckets)
        free(m_buckets);
}

int CFileBackedKeySet::Open()
{
    assert(m_fd < 0);
    assert(m_base == MAP_FAILED);

    m_fd = open(m_filePath.c_str(), O_RDWR | O_CREAT | O_LARGEFILE, 0644);
    if(m_fd < 0)
    {
        log_crit("open %s failed, %m", m_filePath.c_str());
        return -1;
    }

    //discard all content if any
    ftruncate(m_fd, 0);
    ftruncate(m_fd, INIT_FILE_SIZE);

    m_base = (char *)mmap(NULL, INIT_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if(m_base == MAP_FAILED)
    {
        log_crit("mmap failed, %m");
        close(m_fd);
        m_fd = -1;

        return -1;
    }

    meta_info *m = get_meta_info();
    m->size = INIT_FILE_SIZE;
    m->writePos = sizeof(meta_info);

    m_buckets = (hash_node **)calloc(BUCKET_SIZE, sizeof(hash_node *));

    return 0;
}

int CFileBackedKeySet::Load()
{
    assert(m_fd < 0);
    assert(m_base == MAP_FAILED);

    m_fd = open(m_filePath.c_str(), O_RDWR | O_LARGEFILE);
    if(m_fd < 0)
    {
        log_crit("open %s failed, %m", m_filePath.c_str());
        return -1;
    }

    struct stat64 st;
    int rtn = fstat64(m_fd, &st);
    if(rtn < 0)
    {
        log_crit("fstat64 failed, %m");
        close(m_fd);
        m_fd = -1;
        return -1;
    }

    m_base = (char *)mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if(m_base == (char *)MAP_FAILED)
    {
        log_crit("mmap failed, %m");
        close(m_fd);
        m_fd = -1;
        return -1;
    }

    m_buckets = (hash_node **)calloc(BUCKET_SIZE, sizeof(hash_node *));

    char *start = m_base + sizeof(meta_info);
    char *end = m_base + get_meta_info()->writePos;

    //variable key size
    if(m_keySize == 0)
    {
        while(start < end)
        {
            int keyLen = *(unsigned char *)start;
            insert_to_set(start, keyLen + 1);
            start += keyLen + 1;
        }
    }
    else
    {
        while(start < end)
        {
            insert_to_set(start, m_keySize);
            start += m_keySize;
        }
    }

    return 0;
}

int CFileBackedKeySet::Insert(const char *key)
{
    if(Contains(key))
        return 0;

    int keyLen = m_keySize == 0 ? (*(unsigned char *)key) + 1 : m_keySize;
    char *k = insert_to_file(key, keyLen);
    if(!k)  //remap failed?
        return -1;
    insert_to_set(k, keyLen);
    return 0;
}

bool CFileBackedKeySet::Contains(const char *key)
{
    int keyLen = m_keySize == 0 ? (*(unsigned char *)key) + 1 : m_keySize;
    uint32_t hash = new_hash(key, keyLen) % BUCKET_SIZE;
    hash_node *n = m_buckets[hash];
    while(n)
    {
        char *k = m_base + n->offset;
        if(memcmp(key, k, keyLen) == 0)
            return true;
        n = n->next;
    }
    return false;
}

void CFileBackedKeySet::insert_to_set(const char *key, int len)
{
    uint32_t hash = new_hash(key, len) % BUCKET_SIZE;
    hash_node *n = m_allocator.alloc();
    n->next = m_buckets[hash];
    n->offset = key - m_base;
    m_buckets[hash] = n;
}

char *CFileBackedKeySet::insert_to_file(const char *key, int len)
{
    meta_info *m = get_meta_info();
    if(m->writePos + len > m->size)
    {
        uintptr_t new_size = m->size + GROW_FILE_SIZE;
        if(ftruncate(m_fd, new_size) < 0)
        {
            log_crit("grow file to %p failed, %m", (void *)new_size);
            return NULL;
        }

        char *base = (char *)mremap(m_base, m->size, new_size, MREMAP_MAYMOVE);
        if(base == MAP_FAILED)
        {
            log_crit("mremap failed, %m");
            return NULL;
        }

        m_base = base;
        m = get_meta_info();
        m->size = new_size;
    }

    char *writePos = m_base + m->writePos;
    memcpy(writePos, key, len);
    m->writePos += len;

    return writePos;
}

void CFileBackedKeySet::Clear()
{
    if(m_fd >= 0)
    {
        free(m_buckets);
        m_allocator.reset();
        munmap(m_base, get_meta_info()->size);
        close(m_fd);
        m_buckets = 0;
        m_base = (char *)MAP_FAILED;
        m_fd = -1;

        unlink(m_filePath.c_str());
    }
}

