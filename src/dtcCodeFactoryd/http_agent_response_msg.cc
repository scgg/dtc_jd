/*
 * =====================================================================================
 *
 *       Filename:  http_agent_response_msg.cc
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
#include "http_agent_response_msg.h"
#include "http_agent_error_code.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>



static void httptime(struct tm *gmt, char *szDate)
{
	char szWeekDay[][8] = {"SUN","Mon", "TUE", "WED","THU","FRI","SAT"};
	char szMonths[][8] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
	sprintf(szDate, "Date: %s, %02d %s %04d %02d:%02d:%02d GMT",	
		szWeekDay[gmt->tm_wday], gmt->tm_mday, szMonths[gmt->tm_mon], (gmt->tm_year)+1900,	
		gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
}

CHttpAgentResponseMsg::CHttpAgentResponseMsg():m_nHttpVersion(0),m_isKeepAlive(false),m_nStatusCode(0)
{
}

void CHttpAgentResponseMsg::SetCode(int nCode)
{
	m_nStatusCode = nCode;
}

void CHttpAgentResponseMsg::SetKeepAlive(bool isKeepAlive)
{
	m_isKeepAlive = isKeepAlive;
}

void CHttpAgentResponseMsg::SetBody(const string &strResponse)
{
	m_strResponse = strResponse;
}

void CHttpAgentResponseMsg::SetHttpVersion(int nHttpVersion)
{
	m_nHttpVersion = nHttpVersion;
}


const string & CHttpAgentResponseMsg::Encode()
{
	return Encode(m_nHttpVersion,m_isKeepAlive,m_strResponse);
}

const string & CHttpAgentResponseMsg::Encode(int nHttpVersion,bool isKeepAlive,int nCode)
{
    char bufCode[10];
	sprintf(bufCode,"%d",nCode);
	string body = "";
	if(nCode != 0)  //error happened { "return_code" : xxx, "return_message" :  "xxx", "data" : {  } }
	{
		string strErrMsg = CErrorCode::GetInstance()->GetErrorMessage(nCode);
		body=string("{\"return_code\":") + bufCode + ",\"return_message\":\"" + strErrMsg + "\",\"data\":{}}";
	}

	return Encode(nHttpVersion,isKeepAlive,body);
}


const string & CHttpAgentResponseMsg::Encode(int nHttpVersion,bool isKeepAlive,const string &strResponse)
{
	//sprintf(bufLength,"Content-Length: %d\r\n",body.length());
	time_t tNow;	
	struct tm *gmt;	
	tzset(); 
	tNow = time(NULL);
	gmt = (struct tm *)gmtime(&tNow);	
	char szDate[128] = {0};
	httptime(gmt,szDate);

	m_strHttpResponseMsg = "";
	if(nHttpVersion == 1)
	{
		m_strHttpResponseMsg.append("HTTP/1.1 200 OK\r\n");
		m_strHttpResponseMsg.append("Server: HPS Server/1.0.1\r\n");
		m_strHttpResponseMsg.append(string(szDate) + "\r\n");
		m_strHttpResponseMsg.append("Content-Type: text/html;charset=utf-8\r\n");
		m_strHttpResponseMsg.append("Transfer-Encoding: chunked\r\n");
		if(isKeepAlive)
		{			
			m_strHttpResponseMsg.append("Connection: keep-alive\r\n");
		}
		else
		{
			m_strHttpResponseMsg.append("Connection: close\r\n");
		}
		m_strHttpResponseMsg.append("Cache-Control:no-cache;\r\n\r\n");

		char szBodyLen[16] = {0};
		sprintf(szBodyLen, "%zu", strResponse.length());
		m_strHttpResponseMsg.append(string(szBodyLen) + "\r\n");	
		m_strHttpResponseMsg.append(strResponse);
		m_strHttpResponseMsg.append("\r\n\r\n\r\n");
	}
	else //http1.0
	{
		m_strHttpResponseMsg.append("HTTP/1.0 200 OK\r\n");
		m_strHttpResponseMsg.append("Server: HPS Server/1.0.1\r\n");
		m_strHttpResponseMsg.append(string(szDate) + "\r\n");
		m_strHttpResponseMsg.append("Content-Type: text/html;charset=utf-8\r\n");
		char bufLength[64] = {0};
		sprintf(bufLength,"Content-Length: %zu\r\n",strResponse.length());
		m_strHttpResponseMsg.append(bufLength);
		if(isKeepAlive)
		{
			m_strHttpResponseMsg.append("Connection: keep-alive\r\n");
		}
		m_strHttpResponseMsg.append("Cache-Control:no-cache;\r\n\r\n");
		m_strHttpResponseMsg.append(strResponse);
	}
	
	return m_strHttpResponseMsg;
}
