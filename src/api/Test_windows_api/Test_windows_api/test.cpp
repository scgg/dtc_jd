#include "ttcapi.h"
using namespace  TTC;
#pragma comment(lib,"windows_api.lib")

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
 
/*
    if(iRet == 0)
    iRet = stGetReq.EQ("len",10);//����where������ע���һ��key�ֶβ������������
*/


/*
    if(iRet == 0)
    iRet = stGetReq.Need("workername");//������Ҫselect���ֶΣ�ע���һ��key�ֶβ������������
	if(iRet == 0)
		iRet = stGetReq.Need("salary");
	if(iRet == 0)
		iRet = stGetReq.Need("email");
	if(iRet == 0)
		iRet = stGetReq.Need("EmployedDate");
	if(iRet == 0)
		iRet = stGetReq.Need("department");
	if(iRet == 0)
		iRet = stGetReq.Need("sex");
*/

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
    if(iRet != 0)//��������벻Ϊ��˵��ִ�й����г�������������صĺ��������������Ϣ
   { 
    fflush(stdout);
    return(-2); 
    } 
     if(stResult.NumRows() <= 0){ // ���ݲ�����
    printf("uin[%u] data not exist.\n", uiUin);
    return(0); 
    } 
	 
  printf("total row [%u] .\n", stResult.NumRows());
    iRet = stResult.FetchRow();//��ʼ��ȡ����
    if(iRet < 0)
    { 
    printf ("uin[%lu] ttc fetch row error: %d\n", uiUin, iRet);
    fflush(stdout); 
    return(-3); 
    }
    //���һ����ȷ����������������
    printf("data: %s\n", stResult.BinaryValue("data"));//���binary���͵�����
    printf("len: %d\n", stResult.IntValue("len"));//���int���͵�����



/*
    printf("workername:%s\n",stResult.StringValue("workername"));
    printf("salary:%d\n",stResult.IntValue("salary"));
    printf("email:%s\n",stResult.StringValue("email"));
    printf("EmployedDate:%s\n",stResult.StringValue("EmployedDate"));
    printf("department:%s\n",stResult.StringValue("department"));
    printf("sex:%s\n",stResult.StringValue("sex"));
*/



    return(1);
}
 
int Sett(TTC::Server& stServer, unsigned int uiUin, int iOp)
{
    int iRet;
 
    TTC::Request stRequest(&stServer, iOp);
    iRet = stRequest.SetKey(uiUin);
    //���濪ʼ���ø����ֶε�ֵ

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
    if(iRet != 0)//�������,����������롢����׶Ρ�������Ϣ
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
    if(iRet < 0)//�������,����������롢����׶Ρ�������Ϣ
    { 
//		printf ("uin[%lu] ttc execute delete error: %d, error_from:%s, msg:%s\n",
//			uiUin, iRet, stResult.ErrorFrom(), stResult.ErrorMessage());
		fflush(stdout);
		return(-2); 
    }
	
    printf("delete success, result code: %d\n", stResult.ResultCode());
 
    return(0);
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
	TTC::Server stServer; // ֻҪserver����������̨�ᱣ�ֳ�����
	stServer.IntKey(); // ����key����
	stServer.SetTableName("test_dtc");//����ttc�ı�������table.conf��tablenameӦ��һ��
	stServer.SetAddress("10.24.75.19", "10167");//���õ�ttc��ip�Ͷ˿�

	stServer.SetTimeout(5); // �������糬ʱʱ��
	stServer.SetAccessKey("00000018b802bd0b17587523870aaf515e409e27");

	key = 100001;

	//�������д�����Get���� 
	iRet = Gett(stServer, key);
	if(iRet < 0)
		return 0;



	if(iRet >0)//���get�����ݣ�˵��ttc����������ݣ���update��������
	{
		iOp=TTC::RequestUpdate;
		printf("\n%s\n","update");
	}
	else//���û��get�����ݣ�����ttc�в�������
	{
		iOp=TTC::RequestInsert;
		printf("\n%s\n","insert");
	}

	//��iOpΪRequestUpdate�����Ǹ�����������
	//��iOpΪRequestInsert��������������
	iRet = Sett(stServer, key, iOp);
	if(iRet != 0)
		return 0;

	//�����д�����ɾ�����key������
//	iRet = Del(stServer, key);
//	if(iRet != 0)
//	exit(1);


	system("pause");

	return 0;
}