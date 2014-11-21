#ifndef __PLUGIN_OPERATE_H__
#define __PLUGIN_OPERATE_H__

#include "ttcapi.h"
#include <iostream>

class PluginOperate
{
	public:
		PluginOperate() {}
		virtual ~PluginOperate() {}
		virtual int handle(std::string key, int keyType, TTC::Result& rst) { return 0;}
};

class PluginGet : public PluginOperate
{
	public:
		PluginGet() {}
		~PluginGet() {}
		int handle(std::string key, int keyType, TTC::Result& rst);
};


#endif
