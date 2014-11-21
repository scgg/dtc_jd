/*
 * =====================================================================================
 *
 *       Filename:  business_module.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/27/2010 08:36:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "business_module.h"
#include "log.h"
#include "module.h"

CBussinessModuleMgr::CBussinessModuleMgr()
{
}

CBussinessModuleMgr::~CBussinessModuleMgr()
{
}

int CBussinessModuleMgr::AddModule(CModule *module)
{
    if(m_modules.find(module->Id()) != m_modules.end())
        return -1;

    m_modules[module->Id()] = module;

    return 0;
}

void CBussinessModuleMgr::DeleteModule(uint32_t id)
{
    std::map<uint32_t, CModule *>::iterator i = m_modules.find(id);
    if(i != m_modules.end())
    {
        delete i->second;
        m_modules.erase(i);
    }
}

int CBussinessModuleMgr::DumpAllModuleConfig(std::string &message)
{
	log_debug("enter DumpAllModuleConfig");
	std::map<uint32_t, CModule *>::iterator itor=m_modules.begin();
	for (;itor != (m_modules.end());itor++)
	{
		char t[256];
		std::string s;
		sprintf(t,"%d",itor->second->Id());
		s = t;
		//log_error("ModuleId:%d,ListenOn:%s",itor->second->Id(),itor->second->GetListenAddr().c_str());
		message=message+"ModuleId:"+s+"  ListenOn:"+itor->second->GetListenAddr()+"\n";
		itor->second->DumpModuleConfig(message);
	}
	return 0;
}
