#ifndef DTC_PROBER_CONFIG_H
#define DTC_PROBER_CONFIG_H
#include "tinyxml.h"
#include <string>
#include <map>
#include <sys/stat.h>
#include "log.h"

class CProberConfig
{
public:
	CProberConfig(char *config) {
		m_Config = std::string(config);
	}
	~CProberConfig() {
		m_CmdMap.clear();
	}
	int Load() {
		TiXmlDocument configdoc;
		if (!configdoc.LoadFile(m_Config.c_str())) {
			log_error("Load %s failed, errmsg: %s, row: %d, col: %d\n", m_Config.c_str(),
					configdoc.ErrorDesc(), configdoc.ErrorRow(),
					configdoc.ErrorCol());
			return -1;
		}
		TiXmlElement *rootnode = configdoc.RootElement();
		TiXmlElement *proberconf = rootnode->FirstChildElement("PROBER_CONFIG");
		if (NULL == proberconf) {
			log_error("prober conf miss");
			return -1;
		}
		const char *logLevel = proberconf->Attribute("LogLevel");
		if (logLevel)
			m_LogLevel = atoi(logLevel);
		else
			m_LogLevel = 8; // default to log_debug
		_set_log_level_(m_LogLevel);
		const char *worker = proberconf->Attribute("Worker");
		if (worker)
			m_WorkerNum = atoi(worker);
		else
			m_WorkerNum = 2; // default 2 worker thread;
		const char *queue = proberconf->Attribute("Queue");
		if (queue)
			m_QueueSize = atoi(queue);
		else
			m_QueueSize = 3; // default max 3 sessions in queue;
		const char *listenOn = proberconf->Attribute("ListenOn");
		if (!listenOn) {
			log_error("ListenOn miss");
			return -1;
		}
		m_ListenOn = std::string(listenOn);
		return LoadCmdList(rootnode);
	}
	int ReloadCmd() {
		TiXmlDocument configdoc;
		if (!configdoc.LoadFile(m_Config.c_str())) {
			log_error("Load %s failed, errmsg: %s, row: %d, col: %d\n", m_Config.c_str(),
					configdoc.ErrorDesc(), configdoc.ErrorRow(),
					configdoc.ErrorCol());
			return -1;
		}
		TiXmlElement *rootnode = configdoc.RootElement();
		return LoadCmdList(rootnode);
	}
	std::string ListenOn() {
		return m_ListenOn;
	}
	bool CheckModify() {
		struct stat st;
		if (stat(m_Config.c_str(), &st) != 0) {
			log_error("stat config file fail");
			return true;
		}
		if (st.st_mtime == m_LastModify) return false;
		m_LastModify = st.st_mtime;
		log_debug("config file has been modifiled");
		return true;
	}
	std::string CmdCodeToClassName(std::string cmdCode) {
		std::map<std::string, std::string>::const_iterator iter = m_CmdMap.find(cmdCode);
		if (m_CmdMap.end() != iter)
			return iter->second;
		else
			return std::string("");
	}
	int WorkerNum() {
		return m_WorkerNum;
	}
	unsigned int QueueSize() {
		return m_QueueSize;
	}
private:
	int LoadCmdList(TiXmlElement *rootnode) {
		TiXmlElement *cmdconf = rootnode->FirstChildElement("CMDLIST");
		if (!cmdconf) {
			log_error("CMDLIST miss");
			return -1;
		}
		TiXmlElement *cmdnode = cmdconf->FirstChildElement("CMD");
		m_CmdMap.clear();
		while (cmdnode) {
			m_CmdMap.insert(std::pair<std::string, std::string>(cmdnode->Attribute("Id"),
									    cmdnode->Attribute("ClassName")));
			cmdnode = cmdnode->NextSiblingElement("CMD");
		}
		return 0;
	}
	long m_LastModify;
	std::string m_Config;
	int m_LogLevel;
	int m_WorkerNum;
	unsigned int m_QueueSize;
	std::string m_ListenOn;
	std::map<std::string, std::string> m_CmdMap;
};

#endif
