/*
 * =====================================================================================
 *
 *       Filename:  business_module.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/27/2010 08:20:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef AG_BUSINESS_MODULE_H__
#define AG_BUSINESS_MODULE_H__

#include <map>
#include <stdint.h>
#include "sockaddr.h"
#include "poller.h"
#include <string>

class CModule;

class CBussinessModuleMgr
{
public:
    CBussinessModuleMgr();
    virtual ~CBussinessModuleMgr();

    int AddModule(CModule *module);
    CModule *FindModule(uint32_t id)
    {
        std::map<uint32_t, CModule *>::iterator i = m_modules.find(id);
        return i == m_modules.end() ? NULL : i->second;
    }
    void DeleteModule(uint32_t id);
    int DumpAllModuleConfig(std::string &message);
private:
    std::map<uint32_t, CModule *> m_modules;
};

#endif
