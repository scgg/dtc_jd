#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "thread.h"


class CWorkingThread : public CThread
{
public:
	CWorkingThread(char *name):CThread(name, CThread::ThreadTypeAsync){}
	~CWorkingThread(){}
	
	virtual void *Process(void)
	{
		printf("thread run\n");
		while(1)
		{
			usleep(1);
		}
	}
};

int main(int argc, char *argv[])
{
	CWorkingThread **workThreads;
	workThreads = new CWorkingThread*[20000];

	for (int i = 0; i < 20000; i++)
	{
		char threadName[256];

		snprintf(threadName, sizeof(threadName), "test@%d", i);

		workThreads[i] = new CWorkingThread(threadName);
		workThreads[i]->InitializeThread();
		
		workThreads[i]->RunningThread();
	}
	
	sleep(10000);
	return 0;
}
