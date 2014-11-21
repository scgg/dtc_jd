#ifndef __CH_VALUE_H_
#define __CH_VALUE_H_

#include <stdint.h>
#include <string.h>

struct CBinary {
// member
	int len;
	char *ptr;

// API
	int IsEmpty(void) const { return len<=0; };
	uint8_t UnsignedValue(int n=0) const { return *(uint8_t *)(ptr+n); }
	int8_t SignedValue(int n=0) const { return *(uint8_t *)(ptr+n); }
	void NextValue(int n=1) { ptr+=n; len-=n; }
	uint8_t UnsignedValueNext(void) { len--; return *(uint8_t *)ptr++;  }
	int8_t SignedValueNext(void) { len--; return *(int8_t *)ptr++;  }

	int operator!() const { return len<=0; }
	int operator*() const { return *(uint8_t *)ptr; }
	uint8_t operator[] (int n) const { return *(uint8_t *)(ptr+n); }
	CBinary& operator += (int n) { ptr+=n; len-=n; return *this; }
	CBinary& operator++() { ptr++; len--; return *this; }
	CBinary operator++(int) {
		CBinary r = { len, ptr };
		ptr++; len--;
		return r;
	}

	int operator< (int n) const { return len < n; }
	int operator<= (int n) const { return len <= n; }
	int operator> (int n) const { return len > n; }
	int operator>= (int n) const { return len >= n; }

	CBinary& Set(const char *v) { ptr=(char *)v; len=v?strlen(v):0; return *this; }
	CBinary& Set(const char *v, int l) { ptr=(char *)v; len = l; return *this; }
	CBinary& SetZ(const char *v) { ptr=(char *)v; len=v?strlen(v)+1:0; return *this; }
	CBinary& operator = (const char *v) { return Set(v); }


	static CBinary Make(const char *v, int l) { CBinary a; a.Set(v,l); return a; }
	static CBinary Make(const char *v) { CBinary a; a.Set(v); return a; }
	static CBinary MakeZ(const char *v) { CBinary a; a.SetZ(v); return a; }

};

struct CArray: public CBinary {
	CArray(int l, char* p){ len = l; ptr = p; }
	CArray(const CBinary& bin){ len = bin.len; ptr = bin.ptr; }
	
#define ADD_V(TYPE) void Add(TYPE val) \
		{ \
			*(TYPE*)(ptr + len) = val; \
			len += sizeof(TYPE); \
		}
	ADD_V(int8_t);
	ADD_V(uint8_t);
	ADD_V(int32_t);
	ADD_V(uint32_t);
	ADD_V(int64_t);
	ADD_V(uint64_t);
#undef 	ADD_V

	void Add(const char* val){
		uint32_t slen=strlen(val);
		*(uint32_t*)(ptr + len) = slen+1;
		memcpy(ptr+len+sizeof(uint32_t), val, slen+1);
		len += sizeof(uint32_t)+slen+1;
	}
	
	void Add(const char* val, int l){
		*(uint32_t*)(ptr + len) = l;
		memcpy(ptr+len+sizeof(uint32_t), val, l);
		len += sizeof(uint32_t)+l;
	}
	
#define GET_V(TYPE) int Get(TYPE& val, int size=0) \
		{ \
			if((unsigned int)len < sizeof(TYPE)) \
				return(-1); \
			val = *(TYPE*)ptr; \
			ptr += sizeof(TYPE); \
			len -= sizeof(TYPE); \
			return(0); \
		}
		
		GET_V(int8_t);
		GET_V(uint8_t);
		GET_V(int32_t);
		GET_V(uint32_t);
		GET_V(int64_t);
		GET_V(uint64_t);
#undef GET_V
		
		int Get(CBinary& bin)
		{
			if((unsigned int)len < sizeof(uint32_t))
				return(-1);
			bin.len = *(uint32_t*)ptr;
			bin.ptr = ptr + sizeof(uint32_t);
			if((unsigned int)len < sizeof(uint32_t)+bin.len)
				return(-2);
			ptr += sizeof(uint32_t)+bin.len;
			len -= sizeof(uint32_t)+bin.len;
			return(0);
		}
};

typedef union CValue {
// member
	int64_t s64;
	uint64_t u64;
	double flt;
	struct CBinary bin;
	struct CBinary str;
	
// API
	CValue(){}
#if !__x86_64__
	CValue(long v) : s64(v) {}
	CValue(unsigned long v) : u64(v) {}
#else
	CValue(long long v) : s64(v) {}
	CValue(unsigned long long v) : u64(v) {}
#endif
	CValue(int32_t v) : s64(v) {}
	CValue(uint32_t v) : u64(v) {}
	CValue(int64_t v) : s64(v) {}
	CValue(uint64_t v) : u64(v) {}
	CValue(float v) : flt(v) {}
	CValue(double v) : flt(v) {}
	CValue(const char *v) { str = CBinary::Make(v); }
	CValue(const char *v, int l) { str = CBinary::Make(v,l); }
	~CValue(){}
#if !__x86_64__
	static CValue Make(long v) { CValue a(v); return a; }
	static CValue Make(unsigned long v) { CValue a(v); return a; }
#else
	static CValue Make(long long v) { CValue a(v); return a; }
	static CValue Make(unsigned long long v) { CValue a(v); return a; }
#endif
	static CValue Make(int32_t v) { CValue a(v); return a; }
	static CValue Make(uint32_t v) { CValue a(v); return a; }
	static CValue Make(int64_t v) { CValue a(v); return a; }
	static CValue Make(uint64_t v) { CValue a(v); return a; }
	static CValue Make(float v) { CValue a(v); return a; }
	static CValue Make(double v) { CValue a(v); return a; }
	static const CValue Make(const char *v) { const CValue a(v); return a; }
	static const CValue Make(const char *v, int l) { const CValue a(v,l); return a; }
#if !__x86_64__
	CValue & Set(long v) { s64 = v; return *this; }
	CValue & Set(unsigned long v) { u64 = v; return *this; }
#else
	CValue & Set(long long v) { s64 = v; return *this; }
	CValue & Set(unsigned long long v) { u64 = v; return *this; }
#endif
	CValue & Set(int32_t v) { s64 = v; return *this; }
	CValue & Set(uint32_t v) { u64 = v; return *this; }
	CValue & Set(int64_t v) { s64 = v; return *this; }
	CValue & Set(uint64_t v) { u64 = v; return *this; }
	CValue & Set(float v) { flt = v; return *this; }
	CValue & Set(double v) { flt = v; return *this; }
	CValue & Set(char *v) { str.Set(v); return *this; }
	CValue & SetZ(char *v) { str.SetZ(v); return *this; }
	CValue & Set(const char *v, int l) { str.Set(v,l); return *this; }

#if !__x86_64__
	CValue & operator =(long v) { s64 = v; return *this; }
	CValue & operator =(unsigned long v) { u64 = v; return *this; }
#else
	CValue & operator =(long long v) { s64 = v; return *this; }
	CValue & operator =(unsigned long long v) { u64 = v; return *this; }
#endif
	CValue & operator =(int32_t v) { s64 = v; return *this; }
	CValue & operator =(uint32_t v) { u64 = v; return *this; }
	CValue & operator =(int64_t v) { s64 = v; return *this; }
	CValue & operator =(uint64_t v) { u64 = v; return *this; }
	CValue & operator =(float v) { flt = v; return *this; }
	CValue & operator =(double v) { flt = v; return *this; }
	CValue & operator =(char *v) { str.Set(v); return *this; }
} CValue;

extern "C" {
	int mystrcmp (const char *s1, const char *s2, int l);
	static inline char INTERNAL_TO_LOWER(char c) {
		extern unsigned char internal_tolower_table_[];
		return internal_tolower_table_[(unsigned char)c];
	}
};
static inline int IsStringZ(const CBinary &a) {
	return a.len>0 && a.ptr[a.len-1]=='\0'; } // NULL ENDED
static inline int IsStringZ(const CValue &a) {
	return IsStringZ(a.str); }

static inline int StringEqual(const CBinary& a, const char *b) {
	return !mystrcmp(a.ptr, b, a.len); }
static inline int StringEqual(const char* a, const CBinary &b) {
	return StringEqual(b, a); }
static inline int StringEqual(const CValue& a, const char *b) {
	return StringEqual(a.str, b); }
static inline int StringEqual(const char* a, const CValue &b) {
	return StringEqual(a, b.str); }

static inline int StringEqual(const CBinary& a, const CBinary& b) {
	return a.len==b.len && !mystrcmp(a.ptr, b.ptr, a.len); }
static inline int StringEqual(const CValue& a, const CBinary& b) {
	return StringEqual(a.str, b); }
static inline int StringEqual(const CBinary& a, const CValue& b) {
	return StringEqual(a, b.str); }
static inline int StringEqual(const CValue& a, const CValue& b) {
	return StringEqual(a.str, b.str); }

static inline int BinaryEqual(const CBinary& a, const CBinary& b) {
	return a.len==b.len && !memcmp(a.ptr, b.ptr, a.len); }
static inline int BinaryEqual(const CValue& a, const CBinary& b) {
	return StringEqual(a.str, b); }
static inline int BinaryEqual(const CBinary& a, const CValue& b) {
	return StringEqual(a, b.str); }
static inline int BinaryEqual(const CValue& a, const CValue& b) {
	return StringEqual(a.str, b.str); }

#include <sys/select.h>

static inline int FIELD_ISSET(int id, const uint8_t *mask) {
	return FD_ISSET(id, (const fd_set *)mask);
}

static inline void FIELD_SET(int id, uint8_t *mask) {
	FD_SET(id, (fd_set *)mask);
}
static inline void FIELD_CLR(int id, uint8_t *mask) {
	FD_CLR(id, (fd_set *)mask);
}

#define FIELD_ZERO(x)	memset(x, 0, sizeof(x))

struct CBufferChain {
	void Count(int &n, int &l) {
		n = l = 0;
		CBufferChain *bc = this;
		while(bc) {
			n++;
			l += bc->usedBytes;
			bc = bc->nextBuffer;
		}
	}
	CBufferChain *nextBuffer;
	int totalBytes;
	int usedBytes;
	char data[0];
};

#endif
