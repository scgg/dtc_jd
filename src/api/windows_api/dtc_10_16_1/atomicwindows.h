#ifndef  TATOMICU
#define  TATOMICU

#include <stdio.h>
#include <windows.h>

#include <iostream>
using namespace std;
typedef volatile int atomic_t;
#define atomic_set(v,i)		(*(v) = (i))
#define atomic_read(v)		(*(v))

class TAtomicU32
{
private:
	typedef int V;
	atomic_t count;
public:
	~TAtomicU32(void) { }
	TAtomicU32(V v=0) { set(v); }

	inline V get(void) const { return atomic_read((atomic_t *)&count); }
	inline V set(V v) { atomic_set(&count, v); return v; }

	inline V add(V v) {  InterlockedExchangeAdd((LPLONG)&count,v);return count ;}
	inline V sub(V v) {  InterlockedExchangeAdd((LPLONG)&count,v); return count ;}
	inline V inc(void) { return add(1); }
	inline operator V(void) const { return get(); }
	inline V operator++ (void) { return inc(); }
	inline V operator-- (void) { return dec(); }
	inline V dec(void) { return sub(-1); }
	inline V operator++ (int) { return inc()-1; }
	inline V operator-- (int) { return dec()+1; }

	
};



#define DEC_DELETE(pointer) \
	do \
{ \
	if(pointer && pointer->DEC()==0) { \
	delete pointer; \
	pointer = 0; \
	} \
} while(0)


#endif