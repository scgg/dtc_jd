#include <set>
#include <string>
#include <map>

#include "tinyxml.h"
#include "md5.h"
#include "log.h"
#include "config_file.h"

extern char gConfig[];
uint32_t gCurrentConfigureVersion;
std::string gCurrentConfigureCksum;

TiXmlElement *FindModule(TiXmlElement *modules, int id) {
	for (TiXmlElement *child = modules->FirstChildElement("MODULE"); child;
			child = child->NextSiblingElement("MODULE")) {
		if (atoi(child->Attribute("Id")) == id)
			return child;
	}

	return NULL;
}

int ConfigAddModule(int module, const char *listenAddr,
		const char *accessToken) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);

	if (m) {
		log_error("try to add an already exist module %u", module);
		return -1;
	}

	m = new TiXmlElement("MODULE");
	m->SetAttribute("Id", module);
	m->SetAttribute("AccessToken", accessToken);
	m->SetAttribute("ListenOn", listenAddr);

	modules->LinkEndChild(m);
	xml.SaveFile();

	return 0;
}

int ConfigRemoveModule(int module) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);
	if (!m) {
		log_error("module %d not exist in config file", module);
		return -1;
	}

	modules->RemoveChild(m);
	xml.SaveFile();
	return 0;
}

int ConfigAddCacheServer(int module, int id, const char *name, const char *addr,
		const char *hotbak_addr, const char *locate) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);
	if (!m) {
		log_error("module %d not exist in config file", module);
		return -1;
	}

	TiXmlElement *node = new TiXmlElement("CACHEINSTANCE");
	node->SetAttribute("Id", id);
	node->SetAttribute("Name", name);
	node->SetAttribute("Addr", addr);
	node->SetAttribute("HotBackupAddr", hotbak_addr);
	node->SetAttribute("Locate", locate);

	m->LinkEndChild(node);

	xml.SaveFile();
	return 0;
}

TiXmlElement *FindCacheServer(TiXmlElement *module, const char *name) {
	for (TiXmlElement *child = module->FirstChildElement("CACHEINSTANCE");
			child; child = child->NextSiblingElement("CACHEINSTANCE")) {
		if (strcmp(name, child->Attribute("Name")) == 0)
			return child;
	}

	return NULL;
}

int ConfigRemoveCacheServer(int module, const char *name) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);
	if (!m) {
		log_error("module %d not exist in config file", module);
		return -1;
	}

	TiXmlElement *server = FindCacheServer(m, name);
	if (!server) {
		log_error("cache server %s not exist in module %d", name, module);
		return -1;
	}

	m->RemoveChild(server);
	xml.SaveFile();
	return 0;
}

int ConfigChangeCacheServerAddr(int module, const char *name, const char *addr,
		const char *hotbak_addr, const char *locate) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);
	if (!m) {
		log_error("module %d not exist in config file", module);
		return -1;
	}

	TiXmlElement *server = FindCacheServer(m, name);
	if (!server) {
		log_error("cache server %s not exist in module %d", name, module);
		return -1;
	}

	server->SetAttribute("Addr", addr);
	server->SetAttribute("HotBackupAddr", hotbak_addr);
	server->SetAttribute("Locate", locate);
	xml.SaveFile();
	return 0;
}

bool IsModuleValid(TiXmlElement *module, std::string *errorMessage) {
	std::set<std::string> cacheServerNameSet;

	for (TiXmlElement *child = module->FirstChildElement("CACHEINSTANCE");
			child; child = child->NextSiblingElement("CACHEINSTANCE")) {
		if (!child->Attribute("Name") || !child->Attribute("Addr")) {
			*errorMessage = "missing Name or Addr attribute";
			return false;
		}

		if (cacheServerNameSet.find(child->Attribute("Name"))
				!= cacheServerNameSet.end()) {
			*errorMessage = "duplicate cache instance name ";
			*errorMessage += child->Attribute("Name");
			return false;
		}

		cacheServerNameSet.insert(child->Attribute("Name"));
	}

	return true;
}

bool IsConfigValid(TiXmlDocument *xml, std::string *errorMessage) {
	TiXmlElement *root = xml->RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");

	std::set<int> moduleIdSet;
	std::set<std::string> moduleListenAddrSet;

	for (TiXmlElement *m = modules->FirstChildElement("MODULE"); m;
			m = m->NextSiblingElement("MODULE")) {
		if (!m->Attribute("Id") || !m->Attribute("ListenOn")) {
			*errorMessage = "missing attribute Id or ListenOn";
			return false;
		}

		int id = atoi(m->Attribute("Id"));
		if (moduleIdSet.find(id) != moduleIdSet.end()) {
			*errorMessage = "duplicate module id ";
			*errorMessage += m->Attribute("Id");
			return false;
		}

		if (moduleListenAddrSet.find(m->Attribute("ListenOn"))
				!= moduleListenAddrSet.end()) {
			*errorMessage = "duplicate module listen addr ";
			*errorMessage += m->Attribute("ListenOn");
			return false;
		}

		moduleIdSet.insert(id);
		moduleListenAddrSet.insert(m->Attribute("ListenOn"));

		if (!IsModuleValid(m, errorMessage))
			return false;
	}

	return true;
}

bool IsConfigValid(const char *config, std::string *errorMessage) {
	TiXmlDocument xml;
	if (!xml.Parse(config)) {
		*errorMessage = "invalid config file: ";
		*errorMessage += xml.ErrorDesc();
		log_info("Parse xml error:%s", xml.ErrorDesc());
		return false;
	}

	return IsConfigValid(&xml, errorMessage);
}

bool IsConfigValid() {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig))
		return false;

	std::string msg;
	return IsConfigValid(&xml, &msg);
}

void WriteConfig(const char *config) {
	TiXmlDocument xml;
	if (!xml.Parse(config)) {
		log_error("parse config error: %s, config: %s", xml.ErrorDesc(),
				config);
		return;
	}
	xml.SaveFile(gConfig);
}

namespace {
struct Module {
	std::string listenOn;
	std::map<std::string, std::string> servers;
};
}

std::string ConfigGetCksum() {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load config file error!");
		return "ERROR!!";
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");

	std::map<std::string, Module> moduleMap;

	for (TiXmlElement *child = modules->FirstChildElement(); child; child =
			child->NextSiblingElement()) {
		Module &item = moduleMap[child->Attribute("Id")];
		item.listenOn = child->Attribute("ListenOn");
		for (TiXmlElement *s = child->FirstChildElement(); s;
				s = s->NextSiblingElement()) {
			item.servers[s->Attribute("Name")] = s->Attribute("Addr");
		}
	}

	MD5Context md5Context;
	MD5Init(&md5Context);
	for (std::map<std::string, Module>::iterator iter = moduleMap.begin();
			iter != moduleMap.end(); ++iter) {
		MD5Update(&md5Context, (const unsigned char *) iter->first.c_str(),
				iter->first.length());
		Module &item = iter->second;
		MD5Update(&md5Context, (const unsigned char *) item.listenOn.c_str(),
				item.listenOn.length());
		for (std::map<std::string, std::string>::iterator j =
				item.servers.begin(); j != item.servers.end(); ++j) {
			MD5Update(&md5Context, (const unsigned char *) j->first.c_str(),
					j->first.length());
			MD5Update(&md5Context, (const unsigned char *) j->second.c_str(),
					j->second.length());
		}
	}

	unsigned char digest[16];
	MD5Final(digest, &md5Context);

	char str[sizeof(digest) * 2 + 1];
	const char *hex = "0123456789ABCDEF";
	for (size_t i = 0; i < sizeof(digest); ++i) {
		str[i * 2] = hex[digest[i] >> 4];
		str[i * 2 + 1] = hex[digest[i] & 0x0F];
	}
	str[sizeof(digest) * 2] = 0;

	return str;
}

int ConfigSwitchCacheServerAddr(int module, const char *name,
		const char *locate) {
	TiXmlDocument xml;
	if (!xml.LoadFile(gConfig)) {
		log_error("load %s failed, errmsg: %s, row: %d, col: %d", gConfig,
				xml.ErrorDesc(), xml.ErrorRow(), xml.ErrorCol());
		return -1;
	}

	TiXmlElement *root = xml.RootElement();
	TiXmlElement *modules = root->FirstChildElement("BUSINESS_MODULE");
	TiXmlElement *m = FindModule(modules, module);
	if (!m) {
		log_error("module %d not exist in config file", module);
		return -1;
	}

	TiXmlElement *server = FindCacheServer(m, name);
	if (!server) {
		log_error("cache server %s not exist in module %d", name, module);
		return -1;
	}

	server->SetAttribute("Locate", locate);
	xml.SaveFile();
	return 0;
}

