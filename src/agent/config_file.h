#ifndef AGENT_CONFIG_FILE_H__
#define AGENT_CONFIG_FILE_H__

#include <string>

extern uint32_t gCurrentConfigureVersion;
extern std::string gCurrentConfigureCksum;

class TiXmlDocument;

int ConfigAddCacheServer(int module, int id, const char *name, const char *addr,const char *hotbak_addr,const char *locate);

int ConfigRemoveCacheServer(int module, const char *name);

int ConfigChangeCacheServerAddr(int module, const char *name, const char *addr,
		const char *hotbak_addr,const char *locate);

int ConfigAddModule(int module, const char *listenAddr, const char *accessToken);
int ConfigRemoveModule(int module);

bool IsConfigValid(const char *config, std::string *errorMessage);

bool IsConfigValid();

void WriteConfig(const char *config);

std::string ConfigGetCksum();

int ConfigSwitchCacheServerAddr(int module, const char *name,
		const char *locate);

#endif
