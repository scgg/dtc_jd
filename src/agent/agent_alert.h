#ifndef AGENT_ALERT_H
#define AGENT_ALERT_H

#include <vector>
#include <string>
#include "curl_http.h"
#include "json/json.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "log.h"

enum {
	FD_ALARM        =  1, //fd
	MEM_ALARM       =  2,  //memory
	CPU_ALARM       =  3,  //cpu
	NET_ALARM       =  4, // net dev
	PROCESS_ALARM   =  5, //child process core
};

#define THREAD_CPU_STAT_MAX_ERR 6
#define CPU_STAT_INVALID 0
#define CPU_STAT_NOT_INIT 1
#define CPU_STAT_INIT 2
#define CPU_STAT_FINE 3
#define CPU_STAT_ILL 4

struct one_agent_thread_cpu{
	int _fd;        //proc fd thread hold
	int _pid;       //thread pid
	const char * _thread_name;
	double _last_time;
	double _this_time;
	unsigned long long _last_cpu_time;   //previous cpu time of this thread
	unsigned long long _this_cpu_time;   //this cpu time of this thread
	unsigned int _pcpu;             //cpu percentage
	unsigned int _err_times;        //max error calculate time
	unsigned int _status;           //status
	struct one_agent_thread_cpu * _next;
public:
	int init(const char *thread_name, int pid);
	int read_cpu_time(void);
	void cal_pcpu(void);
	void do_stat();
};

class agent_thread_cpu
{
private:
	one_agent_thread_cpu *_stat_list_head;
public:
	agent_thread_cpu()
	{
		_stat_list_head = NULL;
	}
	~agent_thread_cpu()
	{}
	int init(int workerNum);
	void do_stat();
	bool get_cpu_stat(unsigned int &ratio, std::string &thread_name);
private:
	int add_cpu_stat_object(const char *thread_name);

private:
	static const unsigned int cpuLimit = 8000;
};


class AgentAlert
{

public:
	static AgentAlert &GetInstance()
	{
		static AgentAlert instance;
		return instance;
	}

	static  bool splitString(const std::string &str, const std::string &delim, std::vector<std::string> &vec);
	int  DaemonGetFdLimit();
	static bool splitString2(const std::string &str ,std::vector<std::string> &vec);
public:
	/* 扫描进程已打开的文件句柄数 */
	unsigned int ScanProcessOpenningFD(void);

	/*查看网卡信息，网络流量*/
	unsigned int ScanNetDevFlowing(unsigned long &flowPerSecond, unsigned long &flowThreshold);

	/*查看内存使用大小, usedMemory 小于 80%的totalMemory*/
	bool ScanMemoryUsed(unsigned int &total, unsigned int &used);

	/*查看线程cpu使用率*/
	bool ScanCpuThreadStat(unsigned int &ratio, std::string &thread_name);

	bool AlarmToPlatform(long val1, long val2,  const std::string &threadName, unsigned int type = 0);

	void setContacts(const std::string &phone_list)
	{
		mContacts = phone_list;
	}
	void setUrl(const std::string &urlParam)
	{
		mUrl = urlParam;
	}

	std::string getContacts()
	{
		return mContacts;
	}

	std::string getUrl()
	{
		return mUrl;
	}

	void setNetFlow(unsigned int flow)
	{
		mnetFlow = flow;
	}

	unsigned int  getNetFlow()
	{
		return mnetFlow;
	}

	void setEthernet(const std::string &ethernet)
	{
		mEthernet = ethernet;
	}

	std::string getEthernet()
	{
		return mEthernet;
	}
	void setWorkerNum(unsigned int workerNum)
	{
		mWorkerNum = workerNum;
	}

	unsigned int getWorkerNum()
	{
		return mWorkerNum;
	}
	void setFdLimit(unsigned int FdLimit)
	{
		mFdLimit = FdLimit;
	}

	unsigned int getFdLimit()
	{
		return mFdLimit;
	}
	int alertInit()
	{
		int fdNum = DaemonGetFdLimit();
		if(fdNum < 0){
			mFdLimit = 1024;
		}
		else{
			mFdLimit = fdNum;
		}
		int ret = cpu_stat.init(mWorkerNum);

		std::ifstream fin("/proc/net/dev");
		std::string line;
		while(std::getline(fin, line)){
			std::vector<std::string> vec;
			splitString2(line, vec);
			std::stringstream oss;
			oss << mEthernet << ":";
			if(vec.size() > 0 && vec[0] == oss.str()){
				mLastRecv = atoll(vec[1].c_str());
				mLastTran = atoll(vec[9].c_str());
				mLastTime = time(NULL);
				log_info("receiveBytes:[%llu], sendBytes:[%llu]", mLastRecv, mLastTran);
				break;
			}
		}
		return ret;
	}
private:
	AgentAlert() {};
	AgentAlert(const AgentAlert&);
	AgentAlert &operator = (const AgentAlert&);

private:
	std::string mContacts;//delim by ";"
	std::string mUrl;
	unsigned int mnetFlow;
	std::string mEthernet;
	unsigned int mWorkerNum;
	unsigned int mFdLimit;

	//网卡前一次的接收字节数和发送字节数
	unsigned long long mLastRecv;
	unsigned long long mLastTran;
	time_t             mLastTime;
	agent_thread_cpu cpu_stat;
	

};


#endif
