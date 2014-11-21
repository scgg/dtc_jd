#include "ttcapi.h"
using namespace  TTC;
#pragma comment(lib,"_dtc_java_api.lib")

#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

#include <stdio.h>
#include <windows.h>


int Gett(TTC::Server& stServer, int uiUin)
{
    int iRet;
    int iRealLen;
    const char* pchTmp;
 
    TTC::GetRequest stGetReq(&stServer);
    iRet = stGetReq.SetKey(uiUin);

	if(iRet == 0)
		iRet = stGetReq.Need("len");
	if(iRet == 0)
		iRet = stGetReq.Need("data");

    
    if(iRet != 0)
    { 
    printf("get-req set key or need error: %d", iRet);
    fflush(stdout); 
    return(-1); 
    } 
 
    // execute & get result
    TTC::Result stResult; 
    iRet = stGetReq.Execute(stResult);
    if(iRet != 0)//如果返回码不为，说明执行过程中出错，可以用相关的函数来输出错误信息
   { 
    fflush(stdout);
    return(-2); 
    } 
     if(stResult.NumRows() <= 0){ // 数据不存在
    printf("uin[%u] data not exist.\n", uiUin);
    return(0); 
    } 
	 
	printf("total row [%u] .\n", stResult.NumRows());
    iRet = stResult.FetchRow();//开始获取数据
    if(iRet < 0)
    { 
    printf ("uin[%lu] ttc fetch row error: %d\n", uiUin, iRet);
    fflush(stdout); 
    return(-3); 
    }
    //如果一切正确，则可以输出数据了
    printf("data: %s\n", stResult.BinaryValue("data"));//输出binary类型的数据
    printf("len: %d\n", stResult.IntValue("len"));//输出int类型的数据


    return(1);
}
 
int Sett(TTC::Server& stServer, unsigned int uiUin, int iOp)
{
    int iRet;
 
    TTC::Request stRequest(&stServer, iOp);
    iRet = stRequest.SetKey(uiUin);
    //下面开始设置各个字段的值

    if(iRet == 0)
    iRet = stRequest.Set("data", "abc");
	if(iRet == 0)
		iRet = stRequest.Set("len", 123);   


    if(iRet != 0)
    {
    printf("request set error\n");
    }
 
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    printf("num:%d\n",stResult.AffectedRows());
    if(iRet != 0)//如果出错,则输出错误码、错误阶段。错误信息
    { 
 //   printf ("uin[%lu] ttc execute set error: %d, error_from:%s, msg:%s\n",
 //       uiUin, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
    fflush(stdout);
    return(-2); 
    } 
    printf("set success, result code: %d\n", stResult.ResultCode());
 
    return(0);
}
 
int Del(TTC::Server& stServer, unsigned int uiUin)
{
    int iRet;
 
    TTC::Request stRequest(&stServer, TTC::RequestDelete);
    iRet = stRequest.SetKey(uiUin);
    if(iRet != 0)
    {
		printf("request set error\n");
    }
    // execute & get result
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    if(iRet < 0)//如果出错,则输出错误码、错误阶段。错误信息
    { 
//		printf ("uin[%lu] ttc execute delete error: %d, error_from:%s, msg:%s\n",
//			uiUin, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
		fflush(stdout);
		return(-2); 
    }
	
    printf("delete success, result code: %d\n", stResult.ResultCode());
 
    return(0);
}
 



int Purge(TTC::Server& stServer, unsigned int uiUin)
{
	int iRet;

	TTC::Request stRequest(&stServer, TTC::RequestPurge);
	if(iRet=stRequest.SetKey(uiUin) != 0){
		printf("purge::set key error: %d\n", iRet);
		return(-1);
	}

	// execute & get result
	TTC::Result stResult; 
	iRet = stRequest.Execute(stResult);

	if(iRet != 0){ 
		printf("ttc execute purge[%d] error.\n", uiUin);
		return(-3); 
	} 

	printf("purge key[%d] success\n", uiUin);

	return(0);
}


int updateFun(TTC::Server& stServer, unsigned int uiUin)
{
	int retCode;
	TTC::UpdateRequest UpdateReq(&stServer);
	retCode = UpdateReq.SetKey(uiUin);
	if(retCode != 0)
	{
		printf("update-req set key error: %d", retCode);
		fflush(stdout);
		return(-1);
	}
	retCode = UpdateReq.Set("data", "gggg");
	retCode = UpdateReq.Set("len", 3333);

	if(retCode != 0)
	{
		printf("update-req set field error: %d", retCode);
		fflush(stdout);
		return(-1);
	}

	// execute & get result
	TTC::Result stResult;
	retCode = UpdateReq.Execute(stResult);
	printf("retCode:%d\n", retCode);
	if(retCode == 0)
	{
		TTC::GetRequest getReq(&stServer);
		getReq.SetKey(uiUin);
		if(retCode == 0)
			retCode = getReq.Need("data");
		if(retCode == 0)
			retCode = getReq.Need("len");

		if(retCode != 0)
		{
			printf("get-req set key or need error: %d", retCode);
			fflush(stdout);
			return(-1);
		}

		// execute & get result
		retCode = getReq.Execute(stResult);
		if(retCode < 0)
		{ 
			printf ("getReq.Execute get:uin[%lu] error: %d\n", uiUin, retCode);
			fflush(stdout); 
			return(-3); 
		}		

		retCode = stResult.FetchRow();
		if(retCode < 0)
		{ 
			printf ("uin[%lu] ttc fetch row error: %d\n", uiUin, retCode);
			fflush(stdout); 
			return(-3); 
		}

		//如果一切正确，则可以输出数据了
		printf("data: %s\n", stResult.BinaryValue("data"));//输出binary类型的数据
		printf("len: %d\n", stResult.IntValue("len"));//输出int类型的数据
	}
	return 0;
}






int main()
{
//	WSADATA wsa={0};
//	WSAStartup(MAKEWORD(2,2),&wsa);

	int iRet;
	int iOp;
	int key;
	const char *name="127.0.0.1";
	sockaddr_in addrSrv;
	addrSrv.sin_addr.S_un.S_addr=inet_addr(name);
	TTC::Server stServer; // 只要server不析构，后台会保持长连接
	stServer.IntKey(); // 声明key类型
	stServer.SetTableName("test_dtc");//设置ttc的表名，与table.conf中tablename应该一样
	stServer.SetAddress("10.24.75.19", "10167");//设置的ttc的ip和端口

	stServer.SetTimeout(5); // 设置网络超时时间
	stServer.SetAccessKey("00000018b802bd0b17587523870aaf515e409e27");

	key = 100001;




	//下面三行代码是Get数据 
	iRet = Gett(stServer, key);
	if(iRet < 0)
		return 0;



	if(iRet >0)//如果get到数据，说明ttc已有相关数据，则update已有数据
	{
		iOp=TTC::RequestUpdate;
		printf("\n%s\n","update");
	}
	else//如果没有get到数据，则向ttc中插入数据
	{
		iOp=TTC::RequestInsert;
		printf("\n%s\n","insert");
	}

	iRet=updateFun(stServer,key);


	//下面三行代码是Get数据 
	iRet = Gett(stServer, key);
	if(iRet < 0)
		return 0;



	if(iRet >0)//如果get到数据，说明ttc已有相关数据，则update已有数据
	{
		iOp=TTC::RequestUpdate;
		printf("\n%s\n","update");
	}
	else//如果没有get到数据，则向ttc中插入数据
	{
		iOp=TTC::RequestInsert;
		printf("\n%s\n","insert");
	}


	//当iOp为RequestUpdate，则是更新已有数据
	//当iOp为RequestInsert，则是新增数据
//	iRet = Sett(stServer, key, iOp);
//	if(iRet != 0)
//		return 0;
/*
	//puge掉缓存数据
	iRet = Purge(stServer, key);
	if(iRet != 0)
		return 0;


	//下面行代码是删除相应key的数据
	iRet = Del(stServer, key);
	if(iRet != 0)
	exit(1);

*/
	system("pause");

	return 0;
}
