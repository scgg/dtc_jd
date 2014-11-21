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
		//�ĺ������̵߳��ã�����֪ͨ���Է�������Դ����
		//iThreadTotal:��֪ʵ�ַ�����������߳������Ա������ԴԤ����
		//iIndex:�ò����������е�--indexѡ���͸��������ѹ������ʵ�ַ���Ҫ���ԵĽӿ�����������ʵ�ַ��䣩
		//sName:�ò����Ǹ��߿�ܲ��Խӿڵ����ƣ������ܽ�������ʱ��ӡ����Ӧ�ӿ���
		virtual void PreparePressure(int iThreadTotal, int iIndex, std::string &sName) = 0;
		//�ĺ����ᱻ���̵߳���
		//iThreadId:�߳�������ѹ������ʵ�ַ�ʹ�øò���������Դ����
		virtual int  Pressure(int iThreadId) = 0;
		//�ĺ������̵߳��ã�����֪ͨ���Է�������Դ����
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
			m_bExit = true;//֪ͨ�߳��˳�
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
		//���һ���߳��ܵĴ������������߳�
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
	unsigned m_iElapse;//�ܺ�ʱ
	timeval m_stStartTime;//ѹ����������ʱ��
	unsigned m_iCurrentRequests;//��ǰ������
	unsigned m_iTotalRequests;//��������
	unsigned m_iConcurrency;//�����������߳���
	unsigned m_iExitThreads;//�˳��߳���
	unsigned m_iMinElapse;//��С����ʱ��
	unsigned m_iMaxElapse;//�������ʱ��
	unsigned m_iFailRequests;//ʧ��������
	unsigned m_iElapse_c;
	unsigned m_iTestBegin;
	unsigned m_iTestEnd;
	unsigned m_iTotalElapse;
	bool m_bExit;//�����˳���־������ʱ������ǿ��
	CMutex m_Mutex;
	std::string m_sName;//ѹ��������������
	IPressure *m_pPressure;
};

#define DTC_PRESSURE_CTRL CSingleton<DTCPresssure>::Instance()
#define DTC_PRESS_INSTANCE_DECLARE(x) x g_pressObject;
#endif
