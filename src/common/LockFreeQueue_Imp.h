#include "LockFreeQueue.h"
#include <sched.h>

template <typename ELEM_T, uint32_t Q_SIZE>
LockFreeQueue<ELEM_T, Q_SIZE>::LockFreeQueue() : 
       m_readIndex(0), 
	   m_writeIndex(0), 
	   m_maximumReadIndex(0)
{

	m_queueSize = 65536;
	if (Q_SIZE > 65536)
	{
		do
		{
			m_queueSize = m_queueSize << 1;
		}
		while (m_queueSize < Q_SIZE);
		m_queueSize = m_queueSize >> 1;
	}
	m_theQueue = new ELEM_T[m_queueSize];
}

template <typename ELEM_T, uint32_t Q_SIZE>
LockFreeQueue<ELEM_T, Q_SIZE>::~LockFreeQueue()
{
	delete[] m_theQueue;
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t LockFreeQueue<ELEM_T, Q_SIZE>::CountToIndex(uint32_t count)
{
	return (count % m_queueSize);
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t LockFreeQueue<ELEM_T, Q_SIZE>::Size()
{
	uint32_t m_currentWriteIndex = m_writeIndex;
	uint32_t m_currentReadIndex = m_readIndex;
	if (m_currentWriteIndex >= m_currentReadIndex)
	{
		return (m_currentWriteIndex - m_currentReadIndex);
	}
	else
	{
		return (m_queueSize + m_currentWriteIndex - m_currentReadIndex);
	}
}

template <typename ELEM_T, uint32_t Q_SIZE>
uint32_t LockFreeQueue<ELEM_T, Q_SIZE>::QueueSize()
{
	return m_queueSize;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool LockFreeQueue<ELEM_T, Q_SIZE>::EnQueue(const ELEM_T& data)
{
	uint32_t currentReadIndex;
	uint32_t currentWriteIndex;
	do
	{
		currentReadIndex = m_readIndex;
		currentWriteIndex = m_writeIndex;
		if (CountToIndex(currentWriteIndex + 1) == CountToIndex(currentReadIndex))
			return false;
	}
	while(false == CAS(&m_writeIndex, currentWriteIndex, currentWriteIndex+1));
	m_theQueue[CountToIndex(currentWriteIndex)] = data;
	while(false == CAS(&m_maximumReadIndex, currentWriteIndex, currentWriteIndex+1))
	{
		sched_yield();
	}
	return true;
}

template <typename ELEM_T, uint32_t Q_SIZE>
bool LockFreeQueue<ELEM_T, Q_SIZE>::DeQueue(ELEM_T& data)
{
	uint32_t currentMaximumReadIndex;
	uint32_t currentReadIndex;
	do
	{
		currentMaximumReadIndex = m_maximumReadIndex;
		currentReadIndex = m_readIndex;
		if (CountToIndex(currentMaximumReadIndex) == CountToIndex(currentReadIndex))
			return false;
		data = m_theQueue[CountToIndex(currentReadIndex)];
		if (true == CAS(&m_readIndex, currentReadIndex, currentReadIndex+1))
			return true;
	}
	while(1);
}
