/*
 * Version.h
 *
 *  Created on: 2014年7月28日
 *      Author: Jiansong
 */

#ifndef VERSION_H_
#define VERSION_H_
#define AGENT_MAIN_VERSION "1.1.0"
#define GIT_VERSION "7bf6dcc"
#define DATE "Mon Oct 27 11:28:01 20"
#define AUTHOR "root"
#define COMPILEDATE "2014年 10月 27日 星期一 15:05:56 CST"


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
