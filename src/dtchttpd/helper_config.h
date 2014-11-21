#ifndef HELPER_CONFIG
#define HELPER_CONFIG

#include <string>

namespace dtchttpd
{

struct HelperConfig
{
	std::string ip;
	std::string user;
	std::string pass;
	int port;
};

struct HelperGroupConfig
{
	HelperConfig helper_config;
	int helper_num;
};

} //dtchttpd

#endif
