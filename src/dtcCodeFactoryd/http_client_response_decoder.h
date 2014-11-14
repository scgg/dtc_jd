/*
 * =====================================================================================
 *
 *       Filename:  http_client_response_decoder.cc
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
#ifndef _HTTP_RESPONSE_Decoder_H_
#define _HTTP_RESPONSE_Decoder_H_
#include <string>
#include <map>

using std::string;
using std::map;
using std::make_pair;

class CHttpResponseDecoder
{
public:
	CHttpResponseDecoder(){}
	int Decode(char *pBuffer, int nLen);
	
	int GetHttpCode(){return m_nHttpCode;}	
	string GetBody(){return m_strBody;}
	string GetHeadValue(const string &strHead);
	void Dump();
	
private:
	int m_nHttpCode;
	string m_strBody;
	map<string,string> m_mapHeadValue;
	
};

#endif
