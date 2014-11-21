#include "config.h"
#include "log.h"
#include <sstream>
#include <stdlib.h>

using namespace dtchttpd;

extern FK_Config g_config;

CConfig::CConfig() : m_machine_num(0), m_db_num(0), m_group_router_name("group_router"), m_group_name("group")
{
}

CConfig::~CConfig()
{
}

int CConfig::ParseConfig()
{
	//base setting
	g_config.GetIntValue("machine_num", m_machine_num);
	g_config.GetIntValue("db_num", m_db_num);
	g_config.GetStringValue("group_router_name", m_group_router_name);
	g_config.GetStringValue("group_name", m_group_name);
	g_config.GetIntValue("db_mod", m_db_mod);
	g_config.GetIntValue("tab_mod", m_tab_mod);
	if (m_machine_num<=0 || m_db_num<=0 || m_db_mod<=0 || m_tab_mod<=0)
	{
		log_error("config error machine number is %d db number is %d", m_machine_num, m_db_num);
		return -1;
	}
	log_info("machine num is %d db num is %d group_router_name is %s group_name is %s", m_machine_num, m_db_num, m_group_router_name.c_str(), m_group_name.c_str());
	//machine setting
	m_helper_group_config = new HelperGroupConfig*[m_machine_num];
	std::string helper_num_name,ip_name,user_name,pass_name,port_name,db_range_name;
	std::string db_range;
	for (int i=0; i<m_machine_num; i++)
	{
		ostringstream oss;
		oss << "machine_" << i << "_";
		helper_num_name = oss.str().append("helper_num");
		ip_name = oss.str().append("ip");
		user_name = oss.str().append("user");
		pass_name = oss.str().append("pass");
		port_name = oss.str().append("port");
		db_range_name = oss.str().append("db_range");
		log_info("machine[%d]: helper_num_name is %s ip_name is %s user_name is %s pass_name is %s port_name is %s",
		         i, helper_num_name.c_str(), ip_name.c_str(), user_name.c_str(), pass_name.c_str(), port_name.c_str());
		m_helper_group_config[i] = new HelperGroupConfig();
		dtchttpd::HelperConfig config;
		g_config.GetIntValue(helper_num_name, m_helper_group_config[i]->helper_num);
		g_config.GetStringValue(ip_name, config.ip);
		g_config.GetStringValue(user_name, config.user);
		g_config.GetStringValue(pass_name, config.pass);
		g_config.GetIntValue(port_name, config.port);
		m_helper_group_config[i]->helper_config = config;
		if (m_helper_group_config[i]->helper_num<=0 || (m_helper_group_config[i]->helper_config).port<=0 || 
		   (m_helper_group_config[i]->helper_config).ip.empty() || (m_helper_group_config[i]->helper_config).user.empty() || 
		   (m_helper_group_config[i]->helper_config).pass.empty())
		{
			log_error("config error HelperGroupConfig[%d]: helper_num %d ip %s user %s pass %s port %d", i, m_helper_group_config[i]->helper_num, (m_helper_group_config[i]->helper_config).ip.c_str(), 
				     (m_helper_group_config[i]->helper_config).user.c_str(), (m_helper_group_config[i]->helper_config).pass.c_str(), (m_helper_group_config[i]->helper_config).port);
			return -1;
		}
		log_info("HelperGroupConfig[%d]: helper_num %d ip %s user %s pass %s port %d", i, m_helper_group_config[i]->helper_num, (m_helper_group_config[i]->helper_config).ip.c_str(), 
				(m_helper_group_config[i]->helper_config).user.c_str(), (m_helper_group_config[i]->helper_config).pass.c_str(), (m_helper_group_config[i]->helper_config).port);
		//db_machine map
		g_config.GetStringValue(db_range_name, db_range);
		int start_index = atoi(db_range.substr(0, db_range.find('-')+1).c_str());
		int end_index = atoi(db_range.substr(db_range.find('-')+1).c_str());
		for (int j=start_index; j<=end_index; j++)
			m_db_machine_map[j] = i;
	}
	return 0;
}

int CConfig::GetMacheineIndex(int db_index)
{
	return m_db_machine_map[db_index];
}

int CConfig::GetMachineNum() const
{
	return m_machine_num;
}

std::string CConfig::GetGroupRouterName() const
{
	return m_group_router_name;
}

std::string CConfig::GetGroupName() const
{
	return m_group_name;
}

dtchttpd::HelperGroupConfig* CConfig::GetHelperGroupConfig(int index) const
{
	return m_helper_group_config[index];
}

int CConfig::GetDBNum() const
{
	return m_db_num;
}

int CConfig::GetDBMod() const
{
	return m_db_mod;
}

int CConfig::GetTabMod() const
{
	return m_tab_mod;
}

