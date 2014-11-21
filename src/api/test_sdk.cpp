#include<dlfcn.h>     //用于动态库管理的系统头文件  
#include <stdio.h>
#include <errno.h>

int main(int argc,char* argv[])
{
	if (argc != 2)
	{
		printf("Usage:%s sofile\n", argv[0]);
		return -2;
	}
	//void(*pTest)();        
	void*pdlHandle = dlopen(argv[1], RTLD_LAZY);
	if(pdlHandle == NULL )    {
		printf("Failed load library, err:%s\n", dlerror());
		return -1;
	}
	char* pszErr = dlerror();
	if(pszErr != NULL)
	{
		printf("%s\n", pszErr);
		return -1;
	}
	//程序结束时关闭动态库
	dlclose(pdlHandle);
	return 0;  
}
