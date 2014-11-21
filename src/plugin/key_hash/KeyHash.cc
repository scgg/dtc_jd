#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
/* 
 * 通用 key-hash 算法
 *
 * 用区间(left, right) 表示取字符串的那个部分来hash
 * ABCDEFG 比如取 CDE， 则用(3, 5)或者(-5, -3)表示
 */

/* to upper-case */
static inline unsigned char internal_case_hash(unsigned char c)
{
	return c & ~((c & 0x40) >> 1);
}

/* simple string hash algrithm */
static inline uint64_t internal_string_hash(const char *ptr, unsigned len)
{
	uint64_t hash = 0x123;
	do {
		unsigned char c = *ptr++;
		c = internal_case_hash(c);
		hash = hash*101 + c;
	} while (--len);

	return hash;
}

extern "C" uint64_t IntHash(const char* key, int len, int left, int right)
{
	if(NULL == key || 0 == len)
		return 0;

	const char * start = key + (left  < 0 ? len+left : left-1);
	const char * end   = key + (right < 0 ? len+right: right-1) + 1;

	/* 边界限定 */
	if(start < key)    /* 左边最大值 */
		start = key;
	if(end > key+len)  /* 右边最大值 */
		end = key+len;

	if(start >= end)
		return 0;

	int  hash_length = end-start;
	char hash_string[hash_length+1];
	switch(hash_length)
	{
		case 0:
			return 0;
		default:
			memcpy(hash_string, start, hash_length);
			hash_string[hash_length] = '\0';
			return strtoll(hash_string, NULL, 10);
	}

	return 0;
}

extern "C" uint64_t StringHash(const char *key, int len ,int left, int right)
{
	if(NULL == key || 0 == len)
		return 0;

	const char * start = key + (left  < 0 ? len+left : left-1);
	const char * end   = key + (right < 0 ? len+right: right-1) + 1;

	/* 边界限定 */
	if(start < key)    /* 左边最大值 */
		start = key;
	if(end > key+len)  /* 右边最大值 */
		end = key+len;

	if(start >= end)
		return 0;

	return internal_string_hash(start, end-start);
}

/* ttc_comments 日志评论的key-hash分布函数 */
extern "C" uint64_t CommentHash(const char *key, int len, int left, int right)
{
	if(len < 200 && len >= 0)
	{
		unsigned int p = 16777619;
		unsigned int hash = 2166136261UL;
		for(int i = 0; i < len; ++i)
			hash = (hash ^ (unsigned char)key[i]) * p;

		hash += hash << 13;
		hash ^= hash >> 7;
		hash += hash << 3;
		hash ^= hash >> 17;
		hash += hash << 5;
		return hash;
	}
	
	return 0;
}


/* ActHash 活动平台自定义key-hash算法 */
extern "C" uint64_t ActHash(const char* key, int len, int left, int right)
{ 
	unsigned long h = 0, g;
	char *arkey = (char *)key;
	char *arEnd = arkey + len;  

	while (arkey < arEnd) {  
		h = (h << 4) + *arkey++;  
		if ((g = (h & 0xF0000000))) {  
			h = h ^ (g >> 24);  
			h = h ^ g;  
		}  
	}  
	return h%10000;  
}

/* VideoHash video视频库自定义key-hash算法 */
extern "C" uint64_t VideoHash(const char* key, int len, int left, int right)
{
    uint32_t id = 0;

    if (len < 6) {
        return (uint64_t) id;
    }

    if (key[0] >= '0' && key[0] <= '9') {
        id = key[0] - '0';
        return (uint64_t) id;
    }

    char buf[5];
    memcpy(buf, &key[1], 4);
    buf[4] = '\0';
    id = atoi(buf);

    return (uint64_t) id;
}

/* LiveHash live自定义key-hash算法 */
extern "C" uint64_t LiveHash(const char* key, int len, int left, int right)
{
# define UINT32_C(v) ((unsigned int) (v))

#if !defined (get16bits)
#define get16bits(d) ((((const uint8_t *)(d))[1] << UINT32_C(8))+((const uint8_t *)(d))[0])
#endif

    const char* data = key;
    uint32_t    hash = len;
    uint32_t    tmp;
    int         rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 2;
    hash += hash >> 15;
    hash ^= hash << 10;

    return hash;
}
