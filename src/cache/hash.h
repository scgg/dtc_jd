#ifndef __TTC_HASH_H
#define __TTC_HASH_H

#include "namespace.h"
#include "singleton.h"
#include "global.h"
#include "node.h"
#include "new_hash.h"

TTC_BEGIN_NAMESPACE

struct _hash
{
    uint32_t hh_size; 	 	// hash 大小
    uint32_t hh_free; 	 	// 空闲的hash数量
    uint32_t hh_node; 	 	// 挂接的node总数量
    uint32_t hh_fixedsize; 	// key大小：变长key时，hh_fixedsize = 0;其他就是其实际长度
    uint32_t hh_buckets[0];	// hash bucket start
};
typedef struct _hash HASH_T;

class CHash
{
    public:
	CHash();
	~CHash();

	static CHash* Instance(){return CSingleton<CHash>::Instance();}
	static void Destroy() { CSingleton<CHash>::Destroy();}

	inline HASH_ID_T NewHashSlot(const char *key)
	{
		//变长key的前一个字节编码的是key的长度
		uint32_t size = _hash->hh_fixedsize ? _hash->hh_fixedsize : *(unsigned char *)key++;

		//目前仅支持1、2、4字节的定长key
		switch(size){
			case sizeof(unsigned char):
				return (*(unsigned char *)key)  % _hash->hh_size;
			case sizeof(unsigned short):
				return (*(unsigned short *)key) % _hash->hh_size;
			case sizeof(unsigned int):
				return (*(unsigned int *)key)   % _hash->hh_size;
		}
	   
		unsigned int h = new_hash(key, size);
		return h % _hash->hh_size;
	}

	inline HASH_ID_T HashSlot(const char *key)
	{
		//变长key的前一个字节编码的是key的长度
		uint32_t size = _hash->hh_fixedsize ? _hash->hh_fixedsize : *(unsigned char *)key++;

		//目前仅支持1、2、4字节的定长key
		switch(size){
			case sizeof(unsigned char):
				return (*(unsigned char *)key)  % _hash->hh_size;
			case sizeof(unsigned short):
				return (*(unsigned short *)key) % _hash->hh_size;
			case sizeof(unsigned int):
				return (*(unsigned int *)key)   % _hash->hh_size;
		}

		unsigned int h = 0, g = 0;
		const char *arEnd=key + size;

		//变长key hash算法, 目前8字节的定长整型key也是作为变长hash的。
		while (key < arEnd)
		{
			h = (h << 4) + *key++;
			if ((g = (h & 0xF0000000)))
			{
				h = h ^ (g >> 24);
				h = h ^ g;
			}
		}
		return h % _hash->hh_size;
	}

	NODE_ID_T&  Hash2Node(const HASH_ID_T);

	const MEM_HANDLE_T Handle() const { return M_HANDLE(_hash);}
	const char *Error() const { return _errmsg;}

	//创建物理内存并格式化
	int Init(const uint32_t hsize, const uint32_t fixedsize);
	//绑定到物理内存
	int Attach(MEM_HANDLE_T handle);
	//脱离物理内存
	int Detach(void);

	uint32_t HashSize() const { return _hash->hh_size; }
	uint32_t FreeBucket() const { return _hash->hh_free; }
	void IncFreeBucket(int v) { _hash->hh_free += v; }
	void IncNodeCnt(int v) { _hash->hh_node += v; }
	
    private:
	HASH_T *_hash;
        char    _errmsg[256];
};

TTC_END_NAMESPACE

#endif
