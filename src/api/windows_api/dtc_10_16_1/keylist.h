#ifndef __CHC_KEYLIST_H
#define __CHC_KEYLIST_H

#include <string.h>
#include <ctype.h>
#include <map>

#include "value.h"
#define MALLOC(x)	malloc(x)
#define FREE(x)		free(x)
#define FREE_IF(x)	do { if((x) != 0) free((void *)(x)); } while(0)
#define FREE_CLEAR(x)	do { if((x) != 0) { free((void *)(x)); (x)=0; } } while(0)
#define REALLOC(p,sz)	({ void *a=realloc(p,sz); if(a) p = (typeof(p))a; a; })
#define CALLOC(x, y)	calloc(x, y)
class NCKeyInfo {
	private:
		struct nocase
		{
			bool operator()(const char * const & a, const char * const & b) const
			{ return stricmp(a, b) < 0; }
		};
		typedef std::map<const char *, int, nocase> namemap_t;
	private:
		namemap_t keyMap;
		const char *keyName[8];
		uint8_t keyType[8];
		int keyCount;

	public:
		// zero is KeyField::None
		void Clear(void) {
			keyCount = 0;
			memset(keyType, 0, sizeof(keyType));
			memset(keyName, 0, sizeof(keyName));
			keyMap.clear();
		}
		NCKeyInfo() {
			keyCount = 0;
			memset(keyType, 0, sizeof(keyType));
			memset(keyName, 0, sizeof(keyName));
		}
		NCKeyInfo(const NCKeyInfo&that) {
			keyCount = that.keyCount;
			memcpy(keyType, that.keyType, sizeof(keyType));
			memcpy(keyName, that.keyName, sizeof(keyName));
			for(int i=0; i<keyCount; i++)
				keyMap[keyName[i]] = i;
		}
		~NCKeyInfo() {}

		int AddKey(const char *name, int type);
		int Equal(const NCKeyInfo &other) const;
		int KeyIndex(const char *n) const;
		const char *KeyName(int n) const { return keyName[n]; }
		int KeyType(int id) const { return keyType[id]; }
		int KeyFields(void) const { return keyCount; }
};

class NCKeyValueList {
public:
	static int KeyValueMax;
public:
	NCKeyInfo *keyinfo;
	CValue *val;
	int fcount[8];
	int kcount;
public:
	NCKeyValueList(void) : keyinfo(NULL), val(NULL), kcount(0) {
		memset(fcount, 0, sizeof(fcount));
	}
	~NCKeyValueList() {
		FREE_CLEAR(val);
	}
	int KeyFields(void) const { return keyinfo->KeyFields(); }
	int KeyType(int id) const { return keyinfo->KeyType(id); }
	int KeyCount(void) const { return kcount; }
	const char * KeyName(int id) const { return keyinfo->KeyName(id); }

	void Unset(void);
	int AddValue(const char *, const CValue &, int);
	CValue &Value(int row, int col) { return val[row*keyinfo->KeyFields()+col]; }
	const CValue &Value(int row, int col) const { return val[row*keyinfo->KeyFields()+col]; }
	CValue & operator()(int row, int col) { return Value(row, col); }
	const CValue & operator()(int row, int col) const { return Value(row, col); }
	int IsFlat(void) const {
		for(int i=1; i<keyinfo->KeyFields(); i++)
			if(fcount[0] != fcount[i])
				return 0;
		return 1;
	}
};

#endif

