#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "ttc_php_api.h"

#define DIM(a) (sizeof(a)/sizeof(a[0]))

int main(void)
{
	int ret;
	void* ptr;
	char key[10][12];
	char value[10][200];
	KV_PAIR kvs[10];
	KV_PAIR kvs1[10];
	
	srand(getpid());
	
	ret = api_ttc_connect(ptr, "127.0.0.1", 5070, 5, 1);
	if(ret != 0){
		printf("connect error: %d\n", ret);
		exit(1);
	}


/*	api_ttc_settable(ptr, "t_data");
	api_ttc_setkey(ptr, "key", TTC_KEY_TYPE_STRING);
	api_ttc_setval(ptr, "data");
	api_ttc_setlen(ptr, "len");*/

	printf("connect success.\n");
	
	memset(value, 0, sizeof(value));
	for(unsigned int i=0; i<DIM(key); i++){
		snprintf(key[i], sizeof(key[i]), "%d", 10000+i);
		for(unsigned int j=0; j<rand()%(sizeof(value[0])-1)+1; j++)
			value[i][j]='a'+rand()%26;
	}
	
	for(unsigned int i=0; i<DIM(key); i++){
		kvs[i].key=kvs1[i].key=key[i];
		kvs[i].value=value[i];
		kvs[i].vlen=strlen(value[i])+1;
		kvs[i].flags=rand();
		kvs[i].expire_time=rand();
		
		ret = api_ttc_set(ptr, kvs+i);
		if(ret != 0){
			printf("set error: %d, %s\n", ret, api_ttc_strerror(ptr));
			api_ttc_release(ptr);
			exit(1);
		}
	}
	printf("set success.\n");
	
	// get
	ret = api_ttc_get(ptr, kvs1, DIM(key));
	if(ret != 0){
		printf("get error: %d, %s\n", ret, api_ttc_strerror(ptr));
		api_ttc_release(ptr);
		exit(1);
	}
	for(unsigned int i=0; i<DIM(key); i++){
		if(kvs1[i].vlen!=kvs[i].vlen || kvs1[i].flags!=kvs[i].flags || kvs1[i].expire_time!=kvs[i].expire_time){
			printf("incorrect data[%d]!\n", i);
			api_ttc_release(ptr);
			exit(1);
		}
		if(memcmp(kvs1[i].value, kvs[i].value, kvs[i].vlen)!=0){
			printf("incorrect value!\n");
			api_ttc_release(ptr);
			exit(1);
		}
	}
	printf("get success.\n");
/*	
	// delete
	ret = api_ttc_delete(ptr, kvs1, DIM(key));
	if(ret != 0){
		printf("delete error: %d, %s\n", ret, api_ttc_strerror(ptr));
		api_ttc_release(ptr);
		exit(1);
	}
	printf("delete success.\n");
*/	

	api_ttc_release(ptr);

	return(0);
}
