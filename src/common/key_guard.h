#include "timerlist.h"
#include "list.h"

class CKeyGuard {
public:
	CKeyGuard(int keysize, int timeout);
	~CKeyGuard();

	void AddKey(unsigned long barHash, const char *ptrKey);
	bool InSet(unsigned long barHash, const char *ptrKey);
private:
	void TryExpire(void);

private:
	enum { HASHBASE=65536 };
	int keySize;
	int timeout;
	struct list_head hashList[HASHBASE];
	struct list_head lru;
	int (*keycmp)(const char *a, const char *b, int ks);

	struct keyslot {
		struct list_head hlist;
		struct list_head tlist;
		unsigned int timestamp;
		char data[0];
	} __attribute__((__aligned__(1)));
};

