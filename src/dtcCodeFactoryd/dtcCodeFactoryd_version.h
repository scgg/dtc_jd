#ifndef DTCCODEFACTORYD_VERSION_H
#define DTCCODEFACTORYD_VERSION_H

#include <iostream>

#define PRG_NAME "dtcCodeFactoryd"
#define CODEFACTORY_MAIN_VERSION "1.0.0"
#define GIT_VERSION "666537f"

std::string GetCurrentVersion()
{
	return  std::string(CODEFACTORY_MAIN_VERSION)+"_r"+std::string(GIT_VERSION);
}

void ShowUsage()
{
	std::cout << PRG_NAME << " Usage:\n";
	std::cout << "-h show userage.\n";
	std::cout << "-v show dtcCodeFactoryd version.\n";
	std::cout << "-p listent port. it should be 9080 now!\n";
	std::cout << "-c conf file name.\n";
	std::cout << "-d debug log open or not.\n";
}

#endif
