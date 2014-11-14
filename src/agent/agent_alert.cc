#include "agent_alert.h"
#include <fstream>
#include <sstream>
#include<dirent.h>
#include<sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include "log.h"
#include "StatMgr.h"
#include <string.h>

#include <stdlib.h>
#include <sys/param.h> //for HZ
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "memcheck.h"
#include "poll_thread.h"

const unsigned int agent_thread_cpu::cpuLimit;
int agent_thread_cpu::init(int workerNum)
{	//for agent
	add_cpu_stat_object("agent");
	for(int i = 0; i < workerNum; ++i){
		std::stringstream oss;
		oss << "agentworker@" << i;
		add_cpu_stat_object(STRDUP(oss.str().c_str()));
	}

	return 0;
}

//function: 建立cpu统计结构体，添加初始化一些基本项
//input:    无
//ouput：   return -1: 初始化失败
//			return  0：初始化成功
int agent_thread_cpu::add_cpu_stat_object(const char *thread_name)
{
	CThread *thread = CThread::FindThreadByName(thread_name);
	if(thread==NULL || thread->Pid() <= 0){
		//log_alert("+++threadname: %s, pid: %u", thread_name, thread->Pid());
		return -1;
	}
	one_agent_thread_cpu * new_cpu_stat = new one_agent_thread_cpu;
	if(!new_cpu_stat){
		log_warning("thread create new cpu stat error");
		return -1;
	}
	new_cpu_stat->_next = _stat_list_head;
	_stat_list_head = new_cpu_stat;

	return new_cpu_stat->init(thread_name, thread->Pid());
}

//function: 初始化一些基本项
//input:    无
//ouput：   return -1: 初始化失败
//          return  0：初始化成功
int one_agent_thread_cpu::init(const char* thread_name, int pid)
{
	_pid = pid;
	_thread_name = thread_name;
	_last_time = 0;
	_this_time = 0;
	_last_cpu_time = 0;
	_this_cpu_time = 0;
	_pcpu = 100;
	_err_times = 0;
	_status = CPU_STAT_INVALID;
	//_next = NULL;
	int mainpid = getpid();
	char pro_stat_file[256];
	snprintf(pro_stat_file, 255, "/proc/%d/task/%d/stat", mainpid, _pid);
	_fd = open(pro_stat_file, O_RDONLY);
	if(_fd < 0){
		log_warning("open proc stat of thread %d failed", _pid);
	}
	else{
		int ret = read_cpu_time();
		if(ret < 0){
			_status = CPU_STAT_NOT_INIT;
			log_warning("thread [%d] cpu stat init error", _pid);
		}
		else{
			_last_cpu_time = _this_cpu_time;
			_last_time = _this_time;
			_status = CPU_STAT_INIT;
		}
	}

	return 0;
}

static inline void skip_word(char ** p)
{
	*p = strchr(*p, ' ');
	*p  = *p + 1;
}

int one_agent_thread_cpu::read_cpu_time(void)
{
	int ret = lseek(_fd, 0, SEEK_SET);
	if(ret < 0){
		log_warning("lseek proc file to begain failed");
		return -1;
	}

	char stat_buffer[1024];
	ret = read(_fd, stat_buffer, 1023);
	if(ret < 0){
		log_warning("proc file read errro");
		return -1;
	}
	char * p = stat_buffer;
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

void one_agent_thread_cpu::cal_pcpu(void)
{
	double time_diff = _this_time - _last_time;
	time_diff *= HZ;

	if(time_diff > 0){
		_pcpu = (int)(10000 * ((_this_cpu_time - _last_cpu_time) / time_diff));
		//log_debug("thread %s[%d] _pcpu=%llu", _thread_name, _pid, (long long)_pcpu);
	}
	//else, keep previous cpu percentage
	_last_cpu_time = _this_cpu_time;
	_last_time = _this_time;
	return;
}

#define READ_CPU_TIME_ERROR log_warning("thread %s[%d] read cpu time error", _thread_name, _pid)
void one_agent_thread_cpu::do_stat()
{
	int ret = 0;
	switch(_status)
	{
	case CPU_STAT_INVALID:
		break;

	case CPU_STAT_NOT_INIT:
		ret = read_cpu_time();
		if(ret < 0){
			READ_CPU_TIME_ERROR;
		}
		else{
			_last_cpu_time = _this_cpu_time;
			_last_time = _this_time;
			_status = CPU_STAT_INIT;
		}
		break;

	case CPU_STAT_INIT:
		ret = read_cpu_time();
		if(ret < 0){
			READ_CPU_TIME_ERROR;
		}
		else{
			cal_pcpu();
			_err_times = 0;
			_status = CPU_STAT_FINE;
		}
		break;

	case CPU_STAT_FINE:
		ret = read_cpu_time();
		if(ret < 0){
			READ_CPU_TIME_ERROR;
			_err_times ++;
			if(_err_times >= THREAD_CPU_STAT_MAX_ERR){
				log_warning("max err, transfer to ill status");
				_pcpu = 10000;
				_status = CPU_STAT_ILL;
			}
		}
		else {
			cal_pcpu();
			_err_times = 0;
		}
		break;

	case CPU_STAT_ILL:
		ret = read_cpu_time();
		if(ret < 0){
			READ_CPU_TIME_ERROR;
		}
		else {
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

void agent_thread_cpu::do_stat()
{
	one_agent_thread_cpu * cpu_stat = _stat_list_head;
	while(cpu_stat){
		cpu_stat->do_stat();
		cpu_stat = cpu_stat->_next;
	}//end of all thread statistic
	return;
}

bool agent_thread_cpu::get_cpu_stat(unsigned int &ratio, std::string &thread_name)
{
	
	for(one_agent_thread_cpu *p = _stat_list_head; p != NULL; p=p->_next){
		if(p->_pcpu > cpuLimit){
			thread_name = p->_thread_name;
			ratio = p->_pcpu;
		}
		if ((ratio > 0) && (ratio % 3 == 0))
			return true;
	}
	if (ratio != 0)
		return true;
	return false;
}


bool AgentAlert::splitString(const std::string &str, const std::string &delim, std::vector<std::string> &vec)
{
	vec.clear();
	if(str.empty()){
		return false;
	}
	if(delim == "\0"){
		vec.push_back(str);
		return true;
	}
	size_t last = 0;
	size_t index = str.find_first_of(delim,last);
	while (index != std::string::npos)
	{
		std::string subStr = str.substr(last, index-last);
		vec.push_back(subStr);
		last = index + delim.size();
		index = str.find_first_of(delim, last);
	}
	if(last < str.size()){
		std::string subStr = str.substr(last,index-last);
		vec.push_back(subStr);
	}
	return true;
}


bool AgentAlert::splitString2(const std::string &str ,std::vector<std::string> &vec)
{
	int index = 0, head = 0, tail = 0;
	bool flag = false;
	while(index < str.size()){
		if(isspace(str[index])){
			if(!flag){
				++index;
			}
			else{
				tail = index;
				std::string temp = str.substr(head, tail-head);
				vec.push_back(str.substr(head, tail-head));
				flag = false;
			}
		}
		else{
			if(!flag){
				head = index;
				flag = true;
			}
			++index;
		}
	}//while
	if(flag){
		vec.push_back(str.substr(head, str.size()-head));
	}
	return true;
}

int AgentAlert::DaemonGetFdLimit(void)
{
	struct rlimit rlim;
	if(getrlimit(RLIMIT_NOFILE, &rlim) == -1)
	{
		log_notice("Query FdLimit failed, errmsg[%s]",strerror(errno));
		return -1;
	}

	return rlim.rlim_cur;
}

/* 扫描进程已打开的文件句柄数 */
unsigned int AgentAlert::ScanProcessOpenningFD(void)
{
	unsigned int count = 0;
	char fd_dir[1024] = {0};
	DIR * dir;
	struct dirent * ptr;

	snprintf(fd_dir, sizeof(fd_dir), "/proc/%d/fd", getpid());

	if((dir =opendir(fd_dir)) == NULL)
	{
		log_warning("open dir :%s failed, msg:%s", fd_dir, strerror(errno));
		return count;
	}

	while((ptr = readdir(dir)) != NULL)
	{
		if(strcasecmp(ptr->d_name, "..") != 0 && strcasecmp(ptr->d_name, ".") != 0)
			count ++;
	}

	closedir(dir);
	
	return count;
}

/*查看网卡信息，网络流量*/
/*
	Inter-|   Receive                                                |  Transmit
	 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
	    lo:  836004    6805    0    0    0     0          0         0   836004    6805    0    0    0     0       0          0
	  eth0: 42966270  388127    1    4    0     0          0         0  2641108   23599    0    0    0     0       0          0
*/
unsigned int AgentAlert::ScanNetDevFlowing(unsigned long &flowPerSecond, unsigned long &flowThreshold)
{
	unsigned long long curRecv = 0, curTran = 0;
	std::ifstream fin("/proc/net/dev");
	std::string line;
	time_t iElapse = 0;
	while(std::getline(fin, line)){
//		int pos=strcspn(line.c_str(),":");
//		if(pos<strlen(line.c_str()))
		if(line.find(mEthernet)!=std::string::npos)
		{
			break;
		}
//		int pos=line.find_first_of(":");
//		int sublen=line.length()-(pos+1);
//		std::string newLine=line.substr(pos+1,sublen);
//		std::vector<std::string> vec;
//		splitString2(newLine, vec);
//		std::stringstream oss;
//		oss << mEthernet << ":";
//		if(vec.size() > 0 && vec[0] == oss.str()){
//			curRecv = atoll(vec[1].c_str());
//			curTran = atoll(vec[9].c_str());
//			iElapse = mLastTime;
//			mLastTime = time(NULL);
//			iElapse = mLastTime - iElapse;
//			//log_alert("receiveBytes:[%llu], sendBytes:[%llu],Elapse:%d", curRecv, curTran, iElapse);
//			break;
//		}
	}
	const char *tline=line.c_str();
	int pos=strcspn(tline,":");
	if(pos<strlen(tline))
	{
		sscanf(tline + pos + 1, "%llu %*u %*u %*u %*u %*u %*u %*u %llu %*u "
				"%*u %*u %*u %*u %*u %*u",
			&curRecv,
			&curTran);
	}
	iElapse = mLastTime;
	mLastTime = time(NULL);
	iElapse = mLastTime - iElapse;

	if (curRecv > mLastRecv && curTran > mLastTran)
		flowPerSecond = ((curRecv - mLastRecv) + (curTran - mLastTran))/(iElapse*1024*1024);
	else if(curRecv < mLastRecv && curTran > mLastTran)
		flowPerSecond = ((curRecv - mLastRecv + 0xFFFFFFFF) + (curTran - mLastTran))/(iElapse*1024*1024);
	else if(curRecv > mLastRecv && curTran < mLastTran)
		flowPerSecond = ((curRecv - mLastRecv) + (curTran - mLastTran + 0xFFFFFFFF))/(iElapse*1024*1024);
	else if(curRecv < mLastRecv && curTran < mLastTran)
		flowPerSecond = ((curRecv - mLastRecv + 0xFFFFFFFF) + (curTran - mLastTran + 0xFFFFFFFF))/(iElapse*1024*1024);

	flowThreshold = mnetFlow/8;

	//log_alert("flow:%lu  flowThreshold:%lu, curRecv", flowPerSecond, flowThreshold);

	mLastRecv = curRecv;
	mLastTran = curTran;
	return 0;
}

/*查看内存使用大小, usedMemory 小于 80%的totalMemory*/
/*
MemTotal:        4130212 kB
MemFree:         1720880 kB
Buffers:           80384 kB
Cached:           790332 kB
SwapCached:            0 kB
Active:          1675072 kB
Inactive:         642784 kB
Active(anon):    1448220 kB
 *
 */
bool AgentAlert::ScanMemoryUsed(unsigned int &total, unsigned int &used)
{
	unsigned int memTotal = 0, memFree = 0, buffers = 0, cached = 0;

	std::ifstream fin("/proc/meminfo");
	std::string line;
	while(std::getline(fin, line) ){
		std::vector<std::string> vec;
		splitString2(line, vec);
		if(vec.size() != 0 ){
			if(strcasecmp(vec[0].c_str(),"MemTotal:") == 0){
				memTotal = atoi(vec[1].c_str());
			}
			else if(strcasecmp(vec[0].c_str(), "MemFree:") == 0){
				memFree = atoi(vec[1].c_str());
			}
			else if(strcasecmp(vec[0].c_str(),"Buffers:") == 0){
				buffers = atoi(vec[1].c_str());
			}
			else if(strcasecmp(vec[0].c_str(), "Cached:") == 0){
				cached = atoi(vec[1].c_str());
			}
		}
	}

	if(memTotal > (memFree + buffers + cached)){
		unsigned int memUsed = memTotal - memFree - buffers - cached;
		total = memTotal;
		used = memUsed;

		//log_alert("+++memUsed: %u, memTotal: %u", memUsed,(memTotal*8)/10);
		if(memUsed > ((memTotal*8)/10)){
			return true;
		}
	}
	return false;
}

bool AgentAlert::ScanCpuThreadStat(unsigned int &ratio, std::string &thread_name)
{
	cpu_stat.do_stat();
	bool ret = cpu_stat.get_cpu_stat(ratio, thread_name);
	return ret;

}

bool AgentAlert::AlarmToPlatform(long val1, long val2,  const std::string &threadName, unsigned int type)
{
	
	Json::Value innerBody;
	innerBody["alarm_list"] = mContacts;
	std::stringstream oss;
	switch(type)
	{
	case FD_ALARM:
		{
			innerBody["alarm_title"] = "Agent fd Alarm";
			oss << "Agent already used FD nums: " << val1 << ",exceed the threshold: " << val2;
			break;
		}
	case MEM_ALARM:
		{
			 innerBody["alarm_title"] = "Agent memory Alarm";
			 oss << "Agent machine already used memory: " << val1 << " KB,exceed the threshold: " << val2 << "KB";
			break;
		}
	case CPU_ALARM:
		{
			 innerBody["alarm_title"] = "Agent cpu Alarm";
			 oss << "Agent cpu alert, thread: " << threadName << " now has reached: " << val1/100 << " percent";
			break;
		}
	case NET_ALARM:
		{
			 innerBody["alarm_title"] = "Agent Net Alarm";
			 oss << "Agent current network flow: " << val1 << "MB/s, exceed the threshold: " << val2 << "MB/s";
			break;
		}
	case PROCESS_ALARM:
		{
			innerBody["alarm_title"] = "Agent process crashed Alarm";
			if(val2 == 1){
				oss<<"Agent child process has crashed, the exit code: "<<val1;
			}
			else if (val2 == 2){
				oss<< "Agent child process has crashed, killed by signal: " << val1;
			}
			else{
				oss<<"Agent child process has crashed!";
			}
			break;
		}
	default:
		return false;
	}//switch
	
	innerBody["alarm_content"] = oss.str().c_str();
	

	Json::Value body;
	body["app_name"] =  "agent_alarm";
	body["alarm_type"] =  "sms";
	body["data"] = innerBody;
	Json::FastWriter writer;
	std::string strBody = writer.write(body);
	log_info("HttpBody = [%s]", strBody.c_str());


	const unsigned int HttpServiceTimeOut = 3;
	CurlHttp curlHttp;
	std::stringstream reqStream;
	reqStream << "req=" << strBody;
	log_info("m_url: %s  ,reqStream = [%s]", mUrl.c_str(),reqStream.str().c_str());
	curlHttp.SetHttpParams("%s", reqStream.str().c_str());
	curlHttp.SetTimeout(HttpServiceTimeOut);

	BuffV buf;
	
	int ret = curlHttp.HttpRequest(mUrl, &buf, false, "application/x-www-form-urlencoded");
	if(ret != 0) {
		log_error("curlHttp.HttpRequest Error! ret=%d", ret);
		return false;
	}

	std::string strRsponse = buf.Ptr();
	log_info("strRsponse:%s", strRsponse.c_str());

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(strRsponse.c_str(), root)){
		log_error("parse Json failed, strRsponse:%s", strRsponse.c_str());
		return false;
	}

	std::string retStatus = root["status"].asString();
	log_info("retStatus:%s", retStatus.c_str());

	if(retStatus == "success"){
		log_info("report to alarm_platform success.");
		return true;
	}
	else{
		std::string strMessage = root["message"].asString();
		log_error("report to alarm_platform failed, strRetCode:%s.", strMessage.c_str());
		return false;
		//exit(-1);
	}

}
