#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <stdint.h>

#define LOCK_FREE_QUEUE_DEFAULT_SIZE 65536
#define CAS(a_ptr, a_oldVal, a_newVal) __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

template <typename ELEM_T, uint32_t Q_SIZE = LOCK_FREE_QUEUE_DEFAULT_SIZE>
class LockFreeQueue
{
public:
	LockFreeQueue();
	~LockFreeQueue();
	bool EnQueue(const ELEM_T& data);
	bool DeQueue(ELEM_T &data);
	uint32_t Size();
	uint32_t QueueSize();
private:
	ELEM_T *m_theQueue;
	uint32_t m_queueSize;
	volatile uint32_t m_readIndex;
	volatile uint32_t m_writeIndex;
	volatile uint32_t m_maximumReadIndex;
	inline uint32_t CountToIndex(uint32_t count);
};

#include "LockFreeQueue_Imp.h"

#endif
