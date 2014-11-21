/*
 * =====================================================================================
 *
 *       Filename:  http_agent_response_msg.h
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
#ifndef _HTTP_AGENT_RESPONSE_MSG_H_
#define _HTTP_AGENT_RESPONSE_MSG_H_
#include <string>
#include <vector>

using std::string;
using std::vector;

class CHttpPluginResponseMsg
{
	public:
		CHttpPluginResponseMsg();
		~CHttpPluginResponseMsg(){}
		void SetCode(int nCode);
		void SetKeepAlive(bool isKeepAlive);
		void SetBody(const string &strResponse);
		void SetHttpVersion(int nHttpVersion);
		const string & Encode();
		const string & Encode(int nHttpVersion,bool isKeepAlive,int nCode);
		const string & Encode(int nHttpVersion,bool isKeepAlive,const string &strResponse);
		
	private:
		int m_nHttpVersion;
		bool m_isKeepAlive;
		int m_nStatusCode;
		string m_strResponse;
		
		string m_strHttpResponseMsg;
		
};
#endif

