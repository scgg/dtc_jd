/*
 * Version.h
 *
 *  Created on: 2014年7月28日
 *      Author: Jiansong
 */

#ifndef VERSION_H_
#define VERSION_H_
#define AGENT_MAIN_VERSION "1.0.1"
#define GIT_VERSION "32e9a0b"
#define DATE "Wed Nov 19 17:09:35 20"
#define AUTHOR "jasonqiu"
#define COMPILEDATE "Wed Nov 19 17:44:20 CST 2014"


std::string GetCurrentVersion()
{
	return  std::string(AGENT_MAIN_VERSION)+"_r"+std::string(GIT_VERSION);
}

std::string GetLastAuthor()
{
	return AUTHOR;
}

std::string GetLastModifyDate()
{
	return DATE;
}

std::string GetCompileDate()
{
	return COMPILEDATE;
}
#endif /* VERSION_H_ */
