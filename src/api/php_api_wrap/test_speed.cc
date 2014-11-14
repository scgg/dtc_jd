#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>

#include "ttc_php_api.h"

#define DIM(a) (sizeof(a)/sizeof(a[0]))

char value[1025];
int count=0;

uint64_t GetTime()
{
	struct timeval val;
	gettimeofday(&val, NULL);
	return val.tv_sec*1000000ULL+val.tv_usec;
}

void do_test(int proc)
{
	int ret;
	void* ptr;
	char key[100];
	KV_PAIR kvs;	
	
	ret = api_ttc_connect(ptr, "192.168.214.62", 10100, 10);
	if(ret != 0){
		printf("connect error: %d\n", ret);
		exit(1);
	}
	
	srand(getpid());
	for(int j=0; j<1024; j++)
		value[j] = rand()%26+'a';
	value[1024] = 0;
	
	for(int i=0; i<1000; i++){
		snprintf(key, sizeof(key), "test_.%d.%d", proc, i);
		kvs.key = key;
		kvs.value = value;
		kvs.vlen = 1024;
		kvs.flags = 0;
		kvs.expire_time = 0;
		
		ret = api_ttc_set(ptr, &kvs);
		if(ret != 0){
			printf("set error: %d, %s\n", ret, api_ttc_strerror(ptr));
			exit(2);
		}
		count++;
	}
	api_ttc_release(ptr);
	
	return;
}

int main(int argc, char* argv[])
{
	int ret;
	void* ptr;
	int proc_num=50;
	
	if(argc>1)
		proc_num = atoi(argv[1]);
		
	uint64_t begin, end;
	for(int proc=0; proc<proc_num; proc++){
		switch(fork()){
			case -1:
				perror("fork error");
				exit(1);
			case 0:
				begin = GetTime();
				do_test(proc);
				end = GetTime();
				printf("pid[%d] set-req-count[%d], use: %d seconds, %d micro-seconds.\n", getpid(), count, (int)((end-begin)/1000000), (int)((end-begin)%1000000));
				break;
			default:
				break;
		}
	}

	return(0);
}
