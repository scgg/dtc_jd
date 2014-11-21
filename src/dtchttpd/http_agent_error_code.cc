/*
 * =====================================================================================
 *
 *       Filename:  http_agent_error_code.cc
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
#include "http_agent_error_code.h"
#include "http_common.h"

CErrorCode *CErrorCode::sm_pInstance = NULL;

CErrorCode* CErrorCode::GetInstance()
{
    if(sm_pInstance==NULL)
        sm_pInstance=new CErrorCode;
    return sm_pInstance;
}

CErrorCode::CErrorCode()
{
	m_mapErrorMsg[ERROR_HEAD_NOT_IMPLEMENTED] = "501 Not Implemented http method";
	m_mapErrorMsg[ERROR_URL_NOT_SUPPORT] = "Not supported url.";
	m_mapErrorMsg[ERROR_DEOCDE_REQUEST_ERROR] = "Decode the Request msg error";
	m_mapErrorMsg[ERROR_HPS_OUTOF_QUEUE_MAXSIZE] = "hps session's Queue is full";
	
	m_mapErrorMsg[ERROR_TRANSFER_REQUEST_FAILED] = "Bad Request";
	m_mapErrorMsg[ERROR_TRANSFER_RESPONSE_FAILED] = "Transfer Response Avenue Packet Error";
	m_mapErrorMsg[ERROR_TRANSFER_NOT_FOUND] = "No Transfer Method Found";
	
	m_mapErrorMsg[ERROR_NO_SOS_AVAILABLE] = "No server available for the url";
	m_mapErrorMsg[ERROR_SOS_CAN_NOT_REACH] = "Server is disconnected";
	m_mapErrorMsg[ERROR_SOS_RESPONSE_TIMEOUT] = "Sos Response Timeout";
	
	m_mapErrorMsg[ERROR_SOS_SEND_FAIL] = "Sos connection send packet fail";
	m_mapErrorMsg[ERROR_SOS_QUEUE_FULL] = "Sos queue is full";
	m_mapErrorMsg[ERROR_UNDEFINED] = "Undefied Error";	
	m_mapErrorMsg[ERROR_SERVER_REJECT] = "Server reject";
	m_mapErrorMsg[ERROR_CHECK_SIGNATURE] = "Server reject, check signature filed";
	m_mapErrorMsg[ERROR_CHECK_AUTHEN] = "Server reject, no authority";
	m_mapErrorMsg[ERROR_OSAP_REQUEST_FAIL] = "Osap request failed";

	m_mapErrorMsg[ERROR_CONNECT_TO_HTTP_SERVER_FAIL] = "Can not connect to http server";
	m_mapErrorMsg[ERROR_SEND_HTTP_REQUEST_FAIL] = "Send Http request fail";	
	m_mapErrorMsg[ERROR_DECODE_HTTP_RESPONSE_FAIL] = "http response packet illigal";
	m_mapErrorMsg[ERROR_HTTP_SERVER_RESPONSE_TIMEOUT] = "http server response time out";
	m_mapErrorMsg[ERROR_HTTP_REQUEST_URL_ILLEGAL] = "Http request url is illegal";
	m_mapErrorMsg[ERROR_HTTP_SERVER_RESPONSE_FAIL] = "Http server response fail, http code is not 200";
}

string CErrorCode::GetErrorMessage(int nCode)
{
	map<int,string>::iterator iter = m_mapErrorMsg.find(nCode);
	if(iter != m_mapErrorMsg.end())
	{
		return iter->second;
	}
	else
	{
		return m_mapErrorMsg[ERROR_UNDEFINED];
	}
}
