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
    uint32_t hh_size; 	 	// hash ��С
    uint32_t hh_free; 	 	// ���е�hash����
    uint32_t hh_node; 	 	// �ҽӵ�node������
    uint32_t hh_fixedsize; 	// key��С���䳤keyʱ��hh_fixedsize = 0;����������ʵ�ʳ���
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
		//�䳤key��ǰһ���ֽڱ������key�ĳ���
		uint32_t size = _hash->hh_fixedsize ? _hash->hh_fixedsize : *(unsigned char *)key++;

		//Ŀǰ��֧��1��2��4�ֽڵĶ���key
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
		//�䳤key��ǰһ���ֽڱ������key�ĳ���
		uint32_t size = _hash->hh_fixedsize ? _hash->hh_fixedsize : *(unsigned char *)key++;

		//Ŀǰ��֧��1��2��4�ֽڵĶ���key
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

		//�䳤key hash�㷨, Ŀǰ8�ֽڵĶ�������keyҲ����Ϊ�䳤hash�ġ�
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

	//���������ڴ沢��ʽ��
	int Init(const uint32_t hsize, const uint32_t fixedsize);
	//�󶨵������ڴ�
	int Attach(MEM_HANDLE_T handle);
	//���������ڴ�
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
