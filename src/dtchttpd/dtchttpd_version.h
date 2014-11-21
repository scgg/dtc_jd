#ifndef DTCHTTPD_VERSION_H
#define DTCHTTPD_VERSION_H

#include <iostream>

#define PRG_NAME        "dtchttpd"
#define MAIN_VERSION    "1"
#define SECOND_VERSION  "4"
#define FIX_VERSION     "0"
#define GIT_VERSION     "fbb85e9055efcbe8af9037ee0f3e4311ef9a365f"

void ShowVersion()
{
	std::cout << PRG_NAME << " ";
	std::cout << MAIN_VERSION << "." << SECOND_VERSION << "." << FIX_VERSION << " ";
	std::cout << GIT_VERSION << std::endl;
}

void ShowUsage()
{
	std::cout << PRG_NAME << " Usage:\n";
	std::cout << "-v show dtchttpd version.\n";
	std::cout << "-p listent port. it should be 9090 now!\n";
	std::cout << "-c conf file name.\n";
	std::cout << "-d debug log open or not.\n";
	std::cout << "-h show userage.\n";
}

#endif
