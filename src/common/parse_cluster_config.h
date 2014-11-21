#ifndef PARSE_CLUSTER_CONFIG_H__
#define PARSE_CLUSTER_CONFIG_H__

#include <vector>
#include <string>
namespace ClusterConfig
{
#define CLUSTER_CONFIG_FILE "../conf/ClusterConfig.xml"

		struct ClusterNode
		{
				std::string name;
				std::string addr;
				bool self;
		};

		bool ParseClusterConfig(std::vector<ClusterNode> *result, const char *buf, int len);
		bool ParseClusterConfig(std::vector<ClusterNode> *result);
		bool SaveClusterConfig(std::vector<ClusterNode> * result);
		bool CheckAndCreate(const char * filename = NULL);
		bool ChangeNodeAddress(std::string servername, std::string newaddress);
}


#endif
