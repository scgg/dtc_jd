#include "dtcCodeFactoryd_alarm.h"
#include "curl_http.h"
#include "json/json.h"
#include "log.h"
#include "fk_config.h"
#include <sstream>
#include "thread.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <stdlib.h>

using namespace dtcCodeFactoryd;

int thread_cpu::init(int core_num)
{
	char threadName[256];
	for (int i=0; i<core_num; i++)
	{
		snprintf(threadName, sizeof(threadName), "worker@%d", i);
		add_cpu_stat_object(threadName, 8000);
	}
	add_cpu_stat_object("join1", 8000);
	add_cpu_stat_object("join1", 8000);
	add_cpu_stat_object("write", 8000);
	return 0;
}

int thread_cpu::add_cpu_stat_object(const char *thread_name, unsigned int cpu_limit)
{
	CThread *thread = CThread::FindThreadByName(thread_name);
	if(thread==NULL || thread->Pid() <= 0)
	{
		log_error("CThread FindThreadByName failed , thread name %s", thread_name);
		return -1;
	}
	one_thread_cpu *new_cpu_stat = new one_thread_cpu;
	if(!new_cpu_stat)
	{
		log_warning("thread create new cpu stat error");
		return -1;
	}
	new_cpu_stat->_next = _stat_list_head;
	_stat_list_head = new_cpu_stat;

	return new_cpu_stat->init(thread_name, thread->Pid(), cpu_limit);
}

int one_thread_cpu::init(const char *thread_name, int pid, unsigned int cpu_limit)
{
	_pid = pid;
	_thread_name = thread_name;
	_last_time = 0;
	_this_time = 0;
	_last_cpu_time = 0;
	_this_cpu_time = 0;
	_pcpu = 100;
	_err_times = 0;
	_cpuLimit = cpu_limit;
	_status = CPU_STAT_INVALID;
	//_next = NULL;
	int mainpid = getpid();
	char pro_stat_file[256];
	snprintf(pro_stat_file, 255, "/proc/%d/task/%d/stat", mainpid, _pid);
	_fd = open(pro_stat_file, O_RDONLY);
	if(_fd < 0)
	{
		log_warning("open proc stat of thread %d failed", _pid);
	}
	else
	{
		int ret = read_cpu_time();
		if(ret < 0)
		{
			_status = CPU_STAT_NOT_INIT;
			log_warning("thread [%d] cpu stat init error", _pid);
		}
		else
		{
			_last_cpu_time = _this_cpu_time;
			_last_time = _this_time;
			_status = CPU_STAT_INIT;
		}
	}

	return 0;
}

static inline void skip_word(char **p)
{
	*p = strchr(*p, ' ');
	*p  = *p + 1;
}

int one_thread_cpu::read_cpu_time()
{
	int ret = lseek(_fd, 0, SEEK_SET);
	if(ret < 0)
	{
		log_warning("lseek proc file to begain failed");
		return -1;
	}

	char stat_buffer[1024];
	ret = read(_fd, stat_buffer, 1023);
	if(ret < 0)
	{
		log_warning("proc file read errro");
		return -1;
	}
	char *p = stat_buffer;
	skip_word(&p); /*skip pid*/
	skip_word(&p); /*skip cmd name*/
	skip_word(&p); /*skip status*/
	skip_word(&p); /*skip ppid*/
	skip_word(&p); /*skip pgrp*/
	skip_word(&p); /*skip session*/
	skip_word(&p); /*skip tty*/
	skip_word(&p); /*skip tty pgrp*/
	skip_word(&p); /*skip flags*/
	skip_word(&p); /*skip min flt*/
	skip_word(&p); /*skip cmin flt*/
	skip_word(&p); /*skip maj flt*/
	skip_word(&p); /*skip cmaj flt*/

	unsigned long long utime = 0;
	unsigned long long stime = 0;
	char * q = strchr(p, ' ');
	*q = 0;
	utime = atoll(p);
	*q = ' ';

	skip_word(&p); /*skip utime*/
	q = strchr(p, ' ');
	*q = 0;
	stime = atoll(p);
	*q = ' ';

	_this_cpu_time = utime + stime;
	struct timeval now;
	gettimeofday(&now, NULL);
	_this_time = now.tv_sec + now.tv_usec * 1e-6;

	return 0;
}

bool thread_cpu::get_cpu_stat()
{
	v_cpu_info.clear();
	for(one_thread_cpu *p = _stat_list_head; p != NULL; p=p->_next)
	{
		cpu_info info;
		if(p->_pcpu > p->_cpuLimit)
		{
			info.thread_name = p->_thread_name;
			info.ratio = p->_pcpu;
			v_cpu_info.push_back(info);
		}
	}
	if (v_cpu_info.size() > 0)
		return true;
	else
		return false;
}

void thread_cpu::do_stat()
{
	one_thread_cpu *cpu_stat = _stat_list_head;
	while(cpu_stat)
	{
		cpu_stat->do_stat();
		cpu_stat = cpu_stat->_next;
	}//end of all thread statistic
	return;
}

void one_thread_cpu::do_stat()
{
	int ret = 0;
	switch(_status)
	{
		case CPU_STAT_INVALID:
			break;

		case CPU_STAT_NOT_INIT:
			ret = read_cpu_time();
			if(ret < 0)
			{
				log_warning("thread %s[%d] read cpu time error", _thread_name, _pid);
			}
			else
			{
				_last_cpu_time = _this_cpu_time;
				_last_time = _this_time;
				_status = CPU_STAT_INIT;
			}
			break;

		case CPU_STAT_INIT:
			ret = read_cpu_time();
			if(ret < 0)
			{
				log_warning("thread %s[%d] read cpu time error", _thread_name, _pid);
			}
			else
			{
				cal_pcpu();
				_err_times = 0;
				_status = CPU_STAT_FINE;
			}
			break;

		case CPU_STAT_FINE:
			ret = read_cpu_time();
			if(ret < 0)
			{
				log_warning("thread %s[%d] read cpu time error", _thread_name, _pid);
				_err_times ++;
				if(_err_times >= THREAD_CPU_STAT_MAX_ERR)
				{
					log_warning("max err, transfer to ill status");
					_pcpu = 10000;
					_status = CPU_STAT_ILL;
				}
			}
			else 
			{
				cal_pcpu();
				_err_times = 0;
			}
			break;

		case CPU_STAT_ILL:
			ret = read_cpu_time();
			if(ret < 0)
			{
				log_warning("thread %s[%d] read cpu time error", _thread_name, _pid);
			}
			else 
			{
				cal_pcpu();
				_err_times = 0;
				_status = CPU_STAT_FINE;
			}
			break;

		default:
			log_warning("unkown cpu stat status");
			break;
	}//end of this thread statistic

	return;
}

void one_thread_cpu::cal_pcpu()
{
	double time_diff = _this_time - _last_time;
	time_diff *= HZ;

	if(time_diff > 0)
	{
		_pcpu = (int)(10000 * ((_this_cpu_time - _last_cpu_time) / time_diff));
		log_info("thread %s[%d] _pcpu=%llu", _thread_name, _pid, (long long)_pcpu);
	}
	//else, keep previous cpu percentage
	_last_cpu_time = _this_cpu_time;
	_last_time = _this_time;
	return;
}

bool Alarm::ReportToPlatform(int alarm_type, uint32_t val1, uint32_t val2)
{
	if (m_strConf.empty())
	{
		log_error("conf file not set");
		return false;
	}
	//parse conf file
	FK_Config config;
    char config_error_msg[256];
    if (config.Init(m_strConf.c_str(), config_error_msg) < 0)
    {
        log_error("init %s error message: %s", m_strConf.c_str(), config_error_msg);
        return false;
    }
	//set value
	config.GetStringValue("report_url", m_strURL);
	config.GetStringValue("report_phone", m_strPhoneNums);
	if (m_strURL.empty() || m_strPhoneNums.empty())
	{
		log_error("url or phone is empty. url:%s phone:%s", m_strURL.c_str(), m_strPhoneNums.c_str());
		return false;
	}
	int timeout = 0;
	config.GetIntValue("report_timeout", timeout);
	if (timeout > 0)
		m_iHttpTimeout = timeout;
	//set alarm
	Json::Value innerBody;
	innerBody["alarm_list"] = m_strPhoneNums;
	std::stringstream oss;
	switch(alarm_type)
	{
		case WAIT_PID_FAIL:
			{
				innerBody["alarm_title"] = "dtcCodeFactoryd wait pid";
				oss << "dtcCodeFactoryd father process wait pid fail";
				break;
			}
		case KILL_BY_SIGNAL:
			{
				innerBody["alarm_title"] = "dtcCodeFactoryd kill by signal";
				oss << "dtcCodeFactoryd child process killed by signal " << val1;
				break;
			}
		case FORK_ERROR:
			{
				innerBody["alarm_title"] = "dtcCodeFactoryd fork error";
				oss << "dtcCodeFactoryd father process fork child process error";
				break;
			}
		case QUEUE_CONGEST:
			{
				if (0 == m_lLastInsertTime)
				{
					m_lLastInsertTime = GetUnixTime();
				}
				else
				{
					if (GetUnixTime() - m_lLastInsertTime < 600) //10 minutes
					{
						log_error("less than 10 minutes");
						return true;
					}
					else
					{
						m_lLastInsertTime = GetUnixTime();
					}
				}
				innerBody["alarm_title"] = "dtcCodeFactoryd queue congest";
				oss << "dtcCodeFactoryd child process queue size is " << val1 << " has use " << val2;
				break;
			}
		case CPU_RATE_HIGH:
			{
				innerBody["alarm_title"] = "dtcCodeFactoryd cpu rate high";
				oss << "dtcCodeFactoryd ";
				for (int i = 0; i < m_threadCpu.GetCpuInfo().size(); i++)
				{
					oss << "thread name: " << m_threadCpu.GetCpuInfo()[i].thread_name 
					    << " cpu rate: "   << m_threadCpu.GetCpuInfo()[i].ratio/100;
				}
				break;
			}
		default:
			{
				log_error("unknown alarm type");
				return false;
			}
	}
	innerBody["alarm_content"] = oss.str();
	Json::Value body;
	body["app_name"] =  "dtcCodeFactoryd_alarm";
	body["alarm_type"] =  "sms";
	body["data"] = innerBody;
	Json::FastWriter writer;
	std::string strBody = writer.write(body);
	log_info("HttpBody = [%s]", strBody.c_str());
	//do http request
	CurlHttp curlHttp;
	std::stringstream reqStream;
	reqStream << "req=" << strBody;
	log_info("m_strURL: %s  ,reqStream = [%s]", m_strURL.c_str(),reqStream.str().c_str());
	curlHttp.SetHttpParams("%s", reqStream.str().c_str());
	curlHttp.SetTimeout(m_iHttpTimeout);
	BuffV buf;
	int ret = curlHttp.HttpRequest(m_strURL, &buf, false, "application/x-www-form-urlencoded");
	if(ret != 0)
	{
		log_error("curl httprequest error ret=%d", ret);
		return false;
	}
	//varify result
	std::string strRsponse = buf.Ptr();
	log_info("strRsponse:%s", strRsponse.c_str());
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(strRsponse, root))
	{
		log_error("parse json failed, strRsponse:%s", strRsponse.c_str());
		return false;
	}
	std::string retStatus = root["status"].asString();
	log_info("retStatus:%s", retStatus.c_str());
	if(retStatus == "success")
	{
		log_info("report to alarm_platform success.");
		return true;
	}
	else
	{
		log_error("report to alarm_platform failed, strRetCode:%s.", root["message"].asString().c_str());
		return false;
	}
}

bool Alarm::ScanCpuThreadStat()
{
	if (GetUnixTime() - m_lLastReportCpuTime < 10)
	{
		log_info("less than 10 seconds, every 10 second report once");
		return false;
	}
	m_lLastReportCpuTime = GetUnixTime();
	m_threadCpu.do_stat();
	bool ret = m_threadCpu.get_cpu_stat();
	return ret;
}
