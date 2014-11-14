#ifndef DTC_PRESSURE_H
#define DTC_PRESSURE_H
#include "multithread.h"
#include "singleton.h"
#include "lock.h"
#include <string>

class DTCPresssure
{
public:
	class IPressure
	{
	public:
		virtual ~IPressure(){}
		//改函数主线程调用，用于通知测试方进行资源分配
		//iThreadTotal:告知实现方框架启动的线程数，以便进行资源预分配
		//iIndex:该参数即命令行的--index选项的透传，告诉压力测试实现方需要测试的接口索引（由是实现分配）
		//sName:该参数是告诉框架测试接口的名称，方便框架结束测试时打印出对应接口名
		virtual void PreparePressure(int iThreadTotal, int iIndex, std::string &sName) = 0;
		//改函数会被对线程调用
		//iThreadId:线程索引，压力测试实现方使用该参数进行资源分配
		virtual int  Pressure(int iThreadId) = 0;
		//改函数主线程调用，用于通知测试方进行资源回收
		virtual void FinishPressure() = 0;
	};

	friend class CSingleton<DTCPresssure>; 

	DTCPresssure(){
		m_iCurrentRequests = 0;
		m_iElapse = 0;
		m_iConcurrency = 1;
		m_iExitThreads = 0;
		m_iMinElapse = 999999999;
		m_iMaxElapse = 0;
		m_bExit = false;
		m_iElapse_c=0;	
		m_iTestBegin = 0;
		m_iTestEnd = 0;
	}
	~DTCPresssure(){

	}
	
	bool start(int nConcurrency, int iRequests, int iIndex){
		m_iConcurrency = nConcurrency;
		m_iTotalRequests = iRequests;
		gettimeofday(&m_stStartTime, NULL);
		if (m_pPressure)
			m_pPressure->PreparePressure(nConcurrency, iIndex, m_sName);
		m_multiThread.init(this, &DTCPresssure::threadRoutine, nConcurrency, (int)(m_iTotalRequests / nConcurrency));
		m_multiThread.start();
		
		timeval time;
		gettimeofday(&time, NULL);				
		m_iTestBegin = 1000000 * time.tv_sec  + time.tv_usec;
	}
	
	int wait(int iTimeLimit){
		timeval stEndTime;
		if (iTimeLimit != 0){
			sleep(iTimeLimit);
			m_bExit = true;//通知线程退出
		}
		
		while(m_iExitThreads != m_iConcurrency)
		{
			gettimeofday(&stEndTime, NULL);
			usleep(10);
		}
		
	//	m_iElapse = (stEndTime.tv_sec - m_stStartTime.tv_sec) * 1000000 + (stEndTime.tv_usec - m_stStartTime.tv_usec);
		
		if (m_pPressure)
			m_pPressure->FinishPressure();
	}
	
	unsigned getElapse(){
		return m_iElapse;
	}
	
	unsigned getRequests(){
		return m_iCurrentRequests;
	}
	
	unsigned getMaxElapse(){
		return m_iMaxElapse;
	}
	
	unsigned getMinElapse(){
		return m_iMinElapse;
	}
	
	unsigned getFailRequests(){
		return m_iFailRequests;
	}
	
	std::string getName(){
		return m_sName;
	}
	unsigned getElapse_c()
	{
		return m_iElapse_c;
	}
	
	int threadRoutine(int iThreadId, int iThreadParam)
	{
		//最后一个线程跑的次数多余其他线程
		timeval stThreadBeginTime;
                gettimeofday(&stThreadBeginTime, NULL);
		unsigned iTotal = (iThreadId == m_iConcurrency - 1) ? (iThreadParam + m_iTotalRequests % m_iConcurrency) : iThreadParam;
		unsigned i = 0;
		unsigned iMinElapse = 999999999;
		unsigned iMaxElapse = 0;
		unsigned iCurrentRequests = 0;
		unsigned iFailRequests = 0;
		unsigned iThreadElapse = 0;
		
		while(i < iTotal && m_bExit == false)
		{
			iCurrentRequests++;
			if (m_pPressure)
			{
				timeval stBeginTime;
				gettimeofday(&stBeginTime, NULL);
				if (m_pPressure->Pressure(iThreadId) != 0)
					iFailRequests++;
				timeval stEndTime;
				gettimeofday(&stEndTime, NULL);
				
				int iElapse = 1000000 * (stEndTime.tv_sec - stBeginTime.tv_sec) + (stEndTime.tv_usec - stBeginTime.tv_usec);
				
				iThreadElapse = iThreadElapse + iElapse;			
				if (iElapse < iMinElapse)
					iMinElapse =  iElapse;
				
				if (iElapse > iMaxElapse)
					iMaxElapse = iElapse ;
			}
			i++;
		}
		
		timeval stThreadEndTime;
                gettimeofday(&stThreadEndTime, NULL);
		CScopedLock scopedLock(m_Mutex);
		if (iMinElapse < m_iMinElapse)
			m_iMinElapse = iMinElapse;
		if (iMaxElapse > m_iMaxElapse)
			m_iMaxElapse = iMaxElapse;
		m_iElapse = iThreadElapse + m_iElapse;
		printf("ThreadId %u, iThreadElapse %u , iElapse = %u\n",iThreadId,  iThreadElapse, m_iElapse);
		m_iElapse_c+=1000000*(stThreadEndTime.tv_sec-stThreadBeginTime.tv_sec)+(stThreadEndTime.tv_usec-stThreadBeginTime.tv_usec);
		m_iExitThreads++;
		m_iCurrentRequests += iCurrentRequests;
		m_iFailRequests += iFailRequests;
	}
	
	void setPressure(IPressure *pressure){
		
		m_pPressure = pressure;
	}
	void join_in()
	{
		m_multiThread.join_in();
		
		timeval time;
		gettimeofday(&time, NULL);				
		m_iTestEnd = 1000000 * time.tv_sec  + time.tv_usec;		
		m_iTotalElapse = m_iTestEnd - m_iTestBegin;
	}
	unsigned getTotalElapse()
	{
		return m_iTotalElapse;
	}
private:
	common_utility::CMultiThread<DTCPresssure> m_multiThread;
	unsigned m_iElapse;//总耗时
	timeval m_stStartTime;//压力测试启动时间
	unsigned m_iCurrentRequests;//当前请求书
	unsigned m_iTotalRequests;//总请求数
	unsigned m_iConcurrency;//并发数，即线程数
	unsigned m_iExitThreads;//退出线程数
	unsigned m_iMinElapse;//最小消耗时间
	unsigned m_iMaxElapse;//最大消耗时间
	unsigned m_iFailRequests;//失败请求数
	unsigned m_iElapse_c;
	unsigned m_iTestBegin;
	unsigned m_iTestEnd;
	unsigned m_iTotalElapse;
	bool m_bExit;//优雅退出标志，用于时间限制强退
	CMutex m_Mutex;
	std::string m_sName;//压力测试用例名称
	IPressure *m_pPressure;
};

#define DTC_PRESSURE_CTRL CSingleton<DTCPresssure>::Instance()
#define DTC_PRESS_INSTANCE_DECLARE(x) x g_pressObject;
#endif
