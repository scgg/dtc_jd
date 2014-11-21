/*
 * =====================================================================================
 *
 *       Filename:  poll_thread_group.cc
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  10/05/2014 17:40:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  linjinming (prudence), prudence@163.com
 *        Company:  JD, China
 *
 * =====================================================================================
 */
#include "poll_thread_group.h"

CPollThreadGroup::CPollThreadGroup(const std::string groupName):threadIndex(0),pollThreads(NULL)
{
	this->groupName = groupName;
}

CPollThreadGroup::CPollThreadGroup(const std::string groupName, int numThreads, int mp):threadIndex(0),pollThreads(NULL)
{
	this->groupName = groupName;
	Start(numThreads, mp);
}

CPollThreadGroup::~CPollThreadGroup()
{
	if (pollThreads != NULL)
	{
		for (int i = 0; i < this->numThreads; i++)
		{
			if (pollThreads[i])
			{
				pollThreads[i]->interrupt();
				delete pollThreads[i];
			}
		}

		delete pollThreads;

		pollThreads = NULL;
	}
}

CPollThread *CPollThreadGroup::GetPollThread()
{
	if (pollThreads != NULL)
		return pollThreads[threadIndex++ % numThreads];

	return NULL;
}

CPollThread * CPollThreadGroup::GetPollThread(int threadIdx)
{
	if(threadIdx>numThreads)
	{
		return NULL;
	}
	else
	{
		return pollThreads[threadIdx];
	}
}

void CPollThreadGroup::Start(int numThreads, int mp)
{
	char threadName[256];
	this->numThreads = numThreads;

	pollThreads = new CPollThread*[this->numThreads];

	for (int i = 0; i < this->numThreads; i++)
	{
		snprintf(threadName, sizeof(threadName), "%s@%d", groupName.c_str(), i);

		pollThreads[i] = new CPollThread(threadName);
		//SetMaxPollersһ��Ҫ��InitializeThreadǰ���ã�������Ч
		pollThreads[i]->SetMaxPollers(mp);
		pollThreads[i]->InitializeThread();
	}
}

void CPollThreadGroup::RunningThreads()
{
	if (pollThreads == NULL)
		return;

	for (int i = 0; i < this->numThreads; i++)
	{
		pollThreads[i]->RunningThread();
	}
}

int CPollThreadGroup::GetPollThreadIndex(CPollThread *thread)
{
	for(int i=0;i<this->numThreads;i++)
	{
		if(thread==pollThreads[i])
			return i;
	}
	return -1;
}

int  CPollThreadGroup::GetPollThreadSize()
{
	return numThreads;
}
