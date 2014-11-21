/*
 * =====================================================================================
 *
 *       Filename:  http_agent_request_msg.h
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
#ifndef _HTTP_PLUGIN_REQUEST_MSG_H_
#define _HTTP_PLUGIN_REQUEST_MSG_H_
#include <string>
#include <vector>
using std::string;
using std::vector;

class CHttpPluginRequestMsg
{
	public:
		CHttpPluginRequestMsg();
		int Decode(char *pBuff, int nLen);
		string GetCommand() const;
		string GetBody() const;
		string GetXForwardedFor()const{return m_strXForwardedFor;}
		bool IsKeepAlive(){return m_isKeepAlive;}
		bool IsXForwardedFor(){return m_isXForwardedFor;}
		int GetHttpVersion(){return m_nHttpVersion;}
		int HandleHeader(void *pPkg, int iBytesRecved, int *piPkgLen);
	private:
		string m_strBody;
		string m_strRequest;
		string m_strXForwardedFor;

		string m_strPath;
		bool m_isKeepAlive;
		bool m_isXForwardedFor;
		int m_nHttpVersion;
};

#endif

