#ifndef FILE_BACKED_KEY_SET_H__
#define FILE_BACKED_KEY_SET_H__

#include <unistd.h>

#include <stdint.h>
#include <string>
#include <vector>

class CFileBackedKeySet
{
public:
    CFileBackedKeySet(const char *file, int keySize);
    ~CFileBackedKeySet();
    
    bool Contains(const char *key);
    int Open();
    int Load();
    int Insert(const char *key);
    void Clear();

private:
    struct hash_node
    {
        hash_node *next;
        uintptr_t offset;
    };

    class hash_node_allocator
    {
    public:
        hash_node_allocator() : m_freeNode(NULL) {}
        ~hash_node_allocator();
        
        hash_node *alloc();
        void free(hash_node *n);
        void reset();

        static const int GROW_COUNT = 4096 * 16;

    private:
        hash_node *m_freeNode;
        std::vector<char *> m_buffs;
    };

    hash_node_allocator m_allocator;

    struct meta_info
    {
        uintptr_t size;
        uintptr_t writePos;
    };

    //4M
    static const int INIT_FILE_SIZE = 4096 * 1024;
    static const int GROW_FILE_SIZE = 1024 * 1024;
    static const int BUCKET_SIZE = 1024 * 1024;

    meta_info *get_meta_info() { return (meta_info *)m_base; }
    void insert_to_set(const char *key, int len);
    char *insert_to_file(const char *key, int len);

    std::string m_filePath;
    int m_fd;
    int m_keySize;
    char *m_base;
    hash_node **m_buckets;
};

#endif
