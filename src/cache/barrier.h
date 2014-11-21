#ifndef __BARRIER_H__
#define __BARRIER_H__

#include <list.h>
#include <lqueue.h>

class CTaskRequest;
class CBarrierUnit;
class CBarrier;

class CBarrier :
	public CListObject<CBarrier>,
	public CLinkQueue<CTaskRequest *>
{
public:
	friend class CBarrierUnit;

	inline CBarrier (CLinkQueue<CTaskRequest *>::allocator *a=NULL) :
		CLinkQueue<CTaskRequest *>(a), key(0)
	{
	}
	inline ~CBarrier (){
	};

	inline unsigned long Key () const { return key; }
	inline void SetKey(unsigned long k) { key = k; }

private:
	unsigned long key;
};

#endif
