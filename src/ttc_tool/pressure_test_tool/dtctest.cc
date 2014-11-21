
#include "dtcpressure.h"
#include "ttcapi.h"


extern char g_szTableName[1024];
extern char g_szIP[1024];
extern char g_szPort[1024];
extern char g_szAccessKey[1024];
extern int iTimeOut;
extern int ikey;
extern int iOpType;
extern int iAPILogLevel;
extern int iCompressLevel;
const int GET_OP = 0;
const int INSERT_OP = 1;
const int UPDATE_OP = 2;
/*ÿ��key�Զ���һ*/
CMutex kenMutex;
static int iKeyBegin = 0;
extern int iKeyOp;

static int GetTestKey()
{
	/*�̶�key*/
	if (0 == iKeyOp)
	{
		return ikey;
	}
	/*����key*/
	else
	{
		do
		{
			CScopedLock scopedLock(kenMutex);
			iKeyBegin++;
		}while(false);
		int key = ikey + iKeyBegin;
		return key;
	}
}

/*�޸ĸú���������ʵ�ֲ�ͬ�����GET��������*/
static int DtcGetTest(TTC::Server& server)
{
	int rst = 0;
	TTC::Result result;  
	TTC::Request request(&server, TTC::RequestGet);		

	request.SetKey(GetTestKey());  /*����Key��ikey��ʼ����������Ŀ��1*/ 
	request.Need("uintData1");  
	request.Need("uintData2");
	request.Need("intData1");  
	request.Need("intData2");
	request.Need("fltData1");  
	request.Need("fltData2");
	request.Need("strData1");  
	request.Need("strData2");
	request.Need("bData1");  
	request.Need("bData2");
	rst = request.Execute(result); 
	if(rst != 0)
	{
		fprintf(stderr, "tdc execute Get key[%d] error: %d, error from :%s,  error msg: %s\n", ikey, rst, result.ErrorFrom(), result.ErrorMessage());
		return rst;
	}
	
	return rst;
}

/*�޸ĸú���������ʵ�ֲ�ͬ�����INSERT��������*/
static int DtcInsertTest(TTC::Server& server)
{
	int rst = 0;
	TTC::Result result;  
	TTC::Request request(&server, TTC::RequestInsert);	
	
	
	request.SetKey(GetTestKey());  /*����Key��ikey��ʼ����������Ŀ��1*/
	
	request.Set("uintData1", 123);
	request.Set("uintData2", 456);
	request.Set("intData1", 789);
	request.Set("intData2", 101112);
	request.Set("fltData1", 89.078);
	request.Set("fltData1", 72.145);
	request.Set("strData1", "hello");
	request.Set("strData2", "world");
	request.Set("bData1", "0xABCDEF9876543210");
	request.Set("bData2", "0xABCDEF9876543210");
	rst = request.Execute(result); 
	if(rst != 0)
	{
		fprintf(stderr, "tdc execute insert  key[%d]  error: %d, error from %s, error msg %s\n", ikey, rst, result.ErrorFrom(), result.ErrorMessage());
		return rst;
	}
	
	return rst;
}

/*�޸ĸú���������ʵ�ֲ�ͬ�����Update��������*/
static int DtcUpdateTest(TTC::Server& server)
{
	int rst = 0;
	TTC::Result result;  
	TTC::Request request(&server, TTC::RequestUpdate);	
	

	request.SetKey(GetTestKey());  
	request.Set("uintData1", 142857);
	request.Set("uintData2", 142857);
	request.Set("intData1", -142857);
	request.Set("intData2", -142857);
	request.Set("fltData1", 0.6180034);
	request.Set("fltData2", 0.6180033);
	request.Set("strData1", "test for update, compare with redis, person: richard, benjamin");
	request.Set("strData2", "test for update, compare with redis, person: richard, benjamin");
	request.Set("bData1", "0xABCDEF9876543210");
	request.Set("bData2", "0xABCDEF9876543210");
	rst = request.Execute(result);  
	if(rst != 0)
	{
		fprintf(stderr, "tdc execute update  key[%d]  error: %d, error from %s, error msg %s\n", ikey, rst, result.ErrorFrom(), result.ErrorMessage());
		return rst;
	}

	return rst;
}

int DtcOpTest(TTC::Server& server)
{

	if (GET_OP == iOpType)
	{
		return DtcGetTest(server);
	}
	
	else if (INSERT_OP == iOpType)
	{
		return DtcInsertTest(server);
	}
	
	else if (UPDATE_OP == iOpType)
	{	
		return DtcUpdateTest(server);
	}
	
	fprintf(stderr, "unkonw optype %d\n", iOpType);
	return -1;
	
}
class DTCTest : public DTCPresssure::IPressure
{
public:
	DTCTest()
	{
	
		m_Server = NULL;
		DTC_PRESSURE_CTRL->setPressure(static_cast<IPressure *>(this));
	}
	
	~DTCTest()
	{
		if (NULL != m_Server)
		{
			
		}
	}
	void InitDtcServer(TTC::Server& Server)
	{
		Server.IntKey();
				
		Server.SetTableName(g_szTableName);
		Server.SetAddress(g_szIP, g_szPort);
		Server.SetTimeout(iTimeOut); 
		Server.SetAccessKey(g_szAccessKey);
		Server.SetCompressLevel(iCompressLevel);
	}
	//�ĺ������̵߳��ã�����֪ͨ���Է�������Դ����
	virtual void PreparePressure(int iThreadTotal, int iIndex, std::string &sName)
	{
		printf("PreparePressure\n");
		TTC::init_log("DtcBenchLog");
		TTC::set_log_level(iAPILogLevel);
		
		if (NULL == m_Server)
		{
			m_Server = new TTC::Server[iThreadTotal];
			for (int iServerLoop = 0 ; iServerLoop < iThreadTotal; iServerLoop++)
			{
				InitDtcServer(m_Server[iServerLoop]);
			}
		}
		
		if (GET_OP == iOpType)
		{
			sName = "DTC_GET_TEST";
		}

		else if (INSERT_OP == iOpType)
		{
			sName = "DTC_INSERT_TEST";
		}

		else if (UPDATE_OP == iOpType)
		{	
			sName = "DTC_UPDATE_TEST";
		}
		 
		
	}
	//�ĺ����ᱻ���̵߳���
	virtual int Pressure(int iThreadId)
	{
		int rst = DtcOpTest(m_Server[iThreadId]);
		return rst;
	}
	//�ĺ������̵߳��ã�����֪ͨ���Է�������Դ����
	virtual void FinishPressure()
	{
		printf("finish\n");
	}
private:
	TTC::Server* m_Server; 
	int    m_iIndex;
	
};

DTC_PRESS_INSTANCE_DECLARE(DTCTest)

