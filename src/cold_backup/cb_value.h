#ifndef __CB_VALUE_H_
#define __CB_VALUE_H_

#include <stdint.h>
#include <string.h>

/* output u64 format */
#if __WORDSIZE == 64
# define UINT64FMT "%lu"
#else
# define UINT64FMT "%llu"
#endif

#if __WORDSIZE == 64
# define INT64FMT "%ld"
#else
# define INT64FMT "%lld"
#endif

namespace CB
{
	struct CBinary {
		int len;
		char *ptr;

		CBinary() : len(0), ptr(NULL) {}
		CBinary(int l, char *s) : len(l), ptr(s) {}
		~CBinary() {}

		int  clear() { len = 0; ptr = NULL; return 0;}

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
			CBinary r(len, ptr);
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
};
#endif
