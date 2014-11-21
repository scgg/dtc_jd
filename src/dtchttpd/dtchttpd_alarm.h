#ifndef DTCHTTPD_ALARM
#define DTCHTTPD_ALARM

#include <string>
#include <stdint.h>
#include <vector>
#include "helper.h"

namespace dtchttpd
{

#define THREAD_CPU_STAT_MAX_ERR 6
#define CPU_STAT_INVALID 0
#define CPU_STAT_NOT_INIT 1
#define CPU_STAT_INIT 2
#define CPU_STAT_FINE 3
#define CPU_STAT_ILL 4

class one_thread_cpu
{
public:
	int init(const char *thread_name, int pid, unsigned int cpu_limit);
	int read_cpu_time(void);
	void cal_pcpu(void);
	void do_stat();

public:
	int _fd;                             //proc fd thread hold
	int _pid;                            //thread pid
	const char * _thread_name;
	double _last_time;
	double _this_time;
	unsigned long long _last_cpu_time;   //previous cpu time of this thread
	unsigned long long _this_cpu_time;   //this cpu time of this thread
	unsigned int _pcpu;                  //cpu percentage
	unsigned int _err_times;             //max error calculate time
	unsigned int _status;                //status
	unsigned int _cpuLimit;
	struct one_thread_cpu *_next;
};

class thread_cpu
{
private:
        struct cpu_info
        {
                unsigned int ratio;
                std::string thread_name;
        };
public:
	thread_cpu()
	{
		_stat_list_head = NULL;
	}
	
	~thread_cpu()
	{
	}
	
	std::vector<cpu_info>& GetCpuInfo()
	{
		return v_cpu_info;
	}
	
	int init(int core_num);
	void do_stat();
	bool get_cpu_stat();
	int add_cpu_stat_object(const char *thread_name, unsigned int cpu_limit);
	
private:
	one_thread_cpu *_stat_list_head;
private:
	std::vector<cpu_info> v_cpu_info;
};

enum ALARM_TYPE
{
	WAIT_PID_FAIL,
	KILL_BY_SIGNAL,
	FORK_ERROR,
	QUEUE_CONGEST,
	CPU_RATE_HIGH,
};

class Alarm
{
public:
	static Alarm& GetInstance()
	{
		static Alarm instance;
		return instance;
	}
	
	void SetConf(std::string conf_file)
	{
		m_strConf = conf_file;
	}
	
	int Init(int core_num)
	{
		int ret = m_threadCpu.init(core_num);
		return ret;
	}
	
	bool ReportToPlatform(int alarm_type, uint32_t val1 = 0, uint32_t val2 = 0);
	bool ScanCpuThreadStat();
	
private:
	Alarm() : m_iHttpTimeout(2),m_lLastInsertTime(0),m_lLastUpdateTime(0),m_lLastReportCpuTime(GetUnixTime()) {}
	Alarm(const Alarm&);
	Alarm& operator=(const Alarm&);

private:
	std::string m_strConf;
	std::string m_strURL;
	std::string m_strPhoneNums;
	int m_iHttpTimeout;
	long m_lLastInsertTime;
	long m_lLastUpdateTime;
	long m_lLastReportCpuTime;
	std::string m_ip;

private:
	thread_cpu m_threadCpu;
};


} //namespace dtchttpd

#endif
