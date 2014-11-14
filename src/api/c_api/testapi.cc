#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>
 
#include "ttcapi.h"
 
int Get(TTC::Server& stServer, unsigned int uiUin)
{
    int iRet;
    int iRealLen;
    const char* pchTmp;
 
    TTC::GetRequest stGetReq(&stServer);
    iRet = stGetReq.SetKey(uiUin);
 
/*
    if(iRet == 0)
    iRet = stGetReq.EQ("len",10);//设置where条件，注意第一个key字段不能在这里出现
*/
    if(iRet == 0)
    iRet = stGetReq.Need("data");//设置需要select的字段，注意第一个key字段不能在这里出现
    if(iRet == 0)
    iRet = stGetReq.Need("len");
    
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
    printf ("uin[%lu] ttc execute get error: %d, error_from:%s, msg:%s\n",
        uiUin,//出错的key是多少
        iRet,//错误码为多少
        stResult.ErrorFrom(),//返回错误阶段
        stResult.ErrorMessage()//返回错误信息
        );
    fflush(stdout);
    return(-2); 
    } 
 
    if(stResult.NumRows() <= 0){ // 数据不存在
    printf("uin[%u] data not exist.\n", uiUin);
    return(0); 
    } 
 
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
    printf("s_value:%d\n",stResult.IntValue("s_value"));
    return(1);
}
 
int Set(TTC::Server& stServer, unsigned int uiUin, int iOp)
{
    int iRet;
 
    TTC::Request stRequest(&stServer, iOp);
    iRet = stRequest.SetKey(uiUin);
    //下面开始设置各个字段的值
    if(iRet == 0)
    iRet = stRequest.Set("len", 12);
    if(iRet == 0)
    iRet = stRequest.Set("data", "abc");
    
    if(iRet != 0)
    {
    printf("request set error\n");
    }
 
    TTC::Result stResult; 
    iRet = stRequest.Execute(stResult);
    printf("num:%d\n",stResult.AffectedRows());
    if(iRet != 0)//如果出错,则输出错误码、错误阶段。错误信息
    { 
    printf ("uin[%lu] ttc execute set error: %d, error_from:%s, msg:%s\n",
        uiUin, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
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
		printf ("uin[%lu] ttc execute delete error: %d, error_from:%s, msg:%s\n",
			uiUin, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
		fflush(stdout);
		return(-2); 
    }
	
    printf("delete success, result code: %d\n", stResult.ResultCode());
 
    return(0);
}
 
int main(void)
{
	int iRet;
	int iOp;
	unsigned int key;
	TTC::Server stServer; // 只要server不析构，后台会保持长连接
	stServer.IntKey(); // 声明key类型
	stServer.SetTableName("ttc_test");//设置ttc的表名，与table.conf中tablename应该一样
	stServer.SetAddress("127.0.0.1", "10100");//设置的ttc的ip和端口
	stServer.SetTimeout(5); // 设置网络超时时间
	stServer.SetAccessKey("0000010184d9cfc2f395ce883a41d7ffc1bbcf4e");
//	TTC::Server dtcserver;
//		dtcserver.SetAddress("127.0.0.1", "9898");
//		dtcserver.SetTimeout(10);
//		dtcserver.SetAccessKey("11s1");
//
//	TTC::MonitorRequest request(&dtcserver);
//		request.SetAdminCode(12);

//		TTC::Result result;
//		int ret = request.Execute(result);
//		if( ret != 0 )
//			printf("\n%s,%d\n","monitor false",ret);


	

	key = 100001;
	
	//下面三行代码是Get数据 
	iRet = Get(stServer, key);
	if(iRet < 0)
		exit(1);
	
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
    iRet = Set(stServer, key, iOp);
    if(iRet != 0)
		exit(1);
    
    //下面行代码是删除香港key的数据
    //iRet = Del(stServer, key);
    //if(iRet != 0)
		//exit(1);


 
    return(0);
}
