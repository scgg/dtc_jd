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
#include "http_client_response_decoder.h"
#include "log.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#ifdef WIN32
#define snprintf _snprintf
#ifndef strncasecmp
#define strncasecmp strnicmp 
#endif
#endif

static void StrToLower(char *pStr)
{
	while ((*pStr) != '\0')
	{
        if ((*pStr)>='A' && (*pStr)<='Z')
		{
            *pStr += 32;
        }
        pStr++;
    }
}

static char* deleteSpace(char *pSrcStr)
{
	char *pBegin = pSrcStr;
	char *pEnd = pSrcStr + strlen(pSrcStr) - 1;
	
	while ((*pBegin) != '\0' && pBegin < pEnd)
	{
        if ((*pBegin) != ' ')
		{
			break;
        }		
        pBegin++;
    }
	
	while(pEnd >= pBegin)
	{
		if ((*pEnd) != ' ')
		{
			break;
        }	
		*pEnd = '\0';
		pEnd--;
	}

	return pBegin > pSrcStr ? pBegin : pSrcStr;
}

int CHttpResponseDecoder::Decode(char *pBuffer, int nLen)
{
	char *pHeadEnd = strstr(pBuffer, "\r\n\r\n");
	if(pHeadEnd == NULL || pHeadEnd-pBuffer > nLen)
	{
		return -1;
	}
	
	char *pBegin = pBuffer;
	char szVersion[8] = {0};
	char szMsg[32] = {0};
	if(3 != sscanf(pBegin, "HTTP/%7s %d %31[^\r\n]",szVersion, &m_nHttpCode, szMsg))
	{
		return -2;
	}	
	pBegin = strstr(pBegin,"\r\n");

	int nBodyLen=0;
	bool isChunked = false;

	while (pBegin != NULL && pBegin-pBuffer <= nLen && pBegin < pHeadEnd) 
    {   
		char szHead[128]={0};
		char szValue[512]={0};
		
		pBegin += 2;
		if(sscanf(pBegin,"%127[^:]:%511[^\r\n]",szHead,szValue)!=2)
		{
			pBegin = strstr(pBegin+2,"\r\n");
			continue;
		}
		char *pHead = deleteSpace(szHead);
		char *pValue = deleteSpace(szValue);
		StrToLower(pHead);
		m_mapHeadValue.insert(make_pair(pHead,pValue));

		pBegin = strstr(pBegin, "\r\n");	
		
		if(0==strncasecmp(pHead,"content-length",14))
		{
			nBodyLen=atoi(pValue);
			continue;
		}
		if((0==strncasecmp(pHead,"transfer-encoding",17))&&(0==strncasecmp(pValue,"chunked",7)))
		{
			isChunked = true;
			continue;
		}		
	}	

	if(isChunked)
	{
		pBegin = pHeadEnd + 2;
		char *pBodyEnd = strstr(pBegin,"\r\n0\r\n");
		while(pBegin != NULL && pBodyEnd != NULL && (pBegin <= pBodyEnd) && (pBegin-pBuffer <= nLen))		
		{ 
			pBegin += 2;
			char szLen[32] = {0};
			int nLength = 0;		
			int nRet = sscanf(pBegin, "%31[^\r\n]", szLen);
			if(nRet < 1 || NULL == (pBegin = strstr(pBegin,"\r\n")))
			{
				return -2;
			}	
				
			sscanf(szLen, "%d", &nLength);
					
			pBegin += 2;	
			if((pBegin + nLength > pBodyEnd+5) || (pBegin + nLength-pBuffer > nLen))
			{
				log_warning("CHttpResponseDecoder::%s http response illegal.   pBuffer[%s], nLen[%d], pBegin[%s],nLength[%d],pBodyEnd[%s]\n",
						__FUNCTION__,pBuffer,nLen,pBegin,nLength,pBodyEnd+5);
				return -3;
			}
			
			m_strBody += string(pBegin , nLength);
			
			pBegin = pBegin + nLength;			
			pBegin = strstr(pBegin,"\r\n");			
		}
	}
	else
	{
		if (nBodyLen>0) 
		{
		    m_strBody = string(pHeadEnd+4,nBodyLen);            
		}
		else
		{
			int nRest = pBuffer + nLen - pHeadEnd -4;
			if(nRest > 0)
			{
				m_strBody = string(pHeadEnd+4,nRest);
			}
			else
			{
				log_warning("CHttpResponseDecoder::%s http response illegal.   pBuffer[%s], nLen[%d], pHeadEnd[%s]\n",__FUNCTION__,pBuffer,nLen,pHeadEnd);
				log_warning("CHttpResponseDecoder::%s::%d  nLen[%d]  [\n%s\n]\n",__FUNCTION__,__LINE__,nLen,pBuffer);
				return -4;
			}
		}
	}
	return 0;
	
}

string CHttpResponseDecoder::GetHeadValue(const string &strHead)
{
	map<string,string>::iterator itr = m_mapHeadValue.find(strHead);
	if(itr != m_mapHeadValue.end())
	{
		return itr->second;
	}
	else
	{
		return "";
	}
}

void CHttpResponseDecoder::Dump()
{
	log_debug("=========CHttpResponseDecoder::DUMP()=======\n");
	map<string,string>::iterator itr;
	for(itr = m_mapHeadValue.begin(); itr != m_mapHeadValue.end(); ++itr)
	{
		log_debug("%s:%s\n", itr->first.c_str(),itr->second.c_str());
	}

	log_debug("http code[%d]\n", m_nHttpCode);
	log_debug("http body:[%s]\n", m_strBody.c_str());
	log_debug("=========~CHttpResponseDecoder::DUMP()=======\n");
}
