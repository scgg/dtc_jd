/*
 * =====================================================================================
 *
 *       Filename:  http_agent_error_code.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  08/11/2010 08:50:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  linjinming (prudence), prudence@163.com
 *        Company:  JD, China
 *
 * =====================================================================================
 */
#ifndef _ERROR_CODE_H_
#define _ERROR_CODE_H_
#include <string>
#include <map>
using std::string;
using std::map;

class CErrorCode
{
public:
	static CErrorCode* GetInstance();
	string GetErrorMessage(int nCode);
	virtual ~CErrorCode(){}
private:
	CErrorCode();
	static CErrorCode *sm_pInstance;
	map<int, string> m_mapErrorMsg;	
};

#endif
