#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "shmem.h"
#include "unix_lock.h"

CSharedMemory::CSharedMemory() :
	m_key(0), m_id(0), m_size(0), m_ptr(NULL), lockfd(-1)
{
}

CSharedMemory::~CSharedMemory()
{
	Detach();
	Unlock();
}

unsigned long CSharedMemory::Open(int key)
{
	if(m_ptr)
		Detach();

	if(key==0)
		return 0;

	m_key = key;
	if ((m_id = shmget(m_key, 0, 0)) == -1) 
		return 0;

	struct shmid_ds ds;

	if(shmctl(m_id, IPC_STAT, &ds) < 0)
		return 0;

	return m_size = ds.shm_segsz;
}

unsigned long CSharedMemory::Create(int key, unsigned long size)
{
	if(m_ptr)
		Detach();

	m_key = key;

	if(m_key==0)
	{
		m_ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if(m_ptr != MAP_FAILED)
			m_size = size;
		else
			m_ptr = NULL;
	} else {
		if ((m_id = shmget(m_key, size, IPC_CREAT | IPC_EXCL |IPC_PERM)) == -1)
			return 0;

		struct shmid_ds ds;

		if(shmctl(m_id, IPC_STAT, &ds) < 0)
			return 0;

		m_size = ds.shm_segsz;
	}
	return m_size;
}

void *CSharedMemory::Attach(int ro)
{
	if(m_ptr)
		return m_ptr;

	m_ptr = shmat(m_id, NULL, ro?SHM_RDONLY:0);
	if (m_ptr == MAP_FAILED )
		m_ptr = NULL;   
	return m_ptr;
}

void CSharedMemory::Detach(void)
{
	if(m_ptr)
	{
		if(m_key==0)
			munmap(m_ptr, m_size);
		else
			shmdt(m_ptr);
	}
	m_ptr = NULL;
}

int CSharedMemory::Lock(void)
{
	if(lockfd > 0 || m_key==0)
		return 0;
	lockfd = UnixSocketLock("tlock-shm-%u", m_key);
	if(lockfd < 0)
		return -1;
	return 0;
}

void CSharedMemory::Unlock(void)
{
	if(lockfd > 0)
	{
		close(lockfd);
		lockfd = -1;
	}
}

int CSharedMemory::Delete(void)
{
	Detach();

	if(shmctl(m_id, IPC_RMID, NULL) < 0)
		return -1;
	return 0;
}
