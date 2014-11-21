#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "helper_config.h"
#include "fk_config.h"
#include <map>

namespace dtchttpd
{


class CConfig
{
public:
	CConfig();
	virtual ~CConfig();
	int ParseConfig();

public:
	int GetMachineNum() const;
	std::string GetGroupRouterName() const;
	std::string GetGroupName() const;
	dtchttpd::HelperGroupConfig* GetHelperGroupConfig(int index) const;
	int GetDBNum() const;
	int GetMacheineIndex(int db_index);
	int GetDBMod() const;
	int GetTabMod() const;

private:
	dtchttpd::HelperGroupConfig **m_helper_group_config;
	int m_machine_num;
	int m_db_num;
	int m_db_mod;
	int m_tab_mod;
	std::string m_group_router_name;
	std::string m_group_name;
	std::map<int,int> m_db_machine_map;
	
};

} //dtchttpd

#endif
