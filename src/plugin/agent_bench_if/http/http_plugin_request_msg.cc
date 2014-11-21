/*
 * =====================================================================================
 *
 *       Filename:  http_agent_request_msg.cc
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
#include "http_plugin_request_msg.h"
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

CHttpPluginRequestMsg::CHttpPluginRequestMsg():m_isKeepAlive(false),m_isXForwardedFor(false),m_nHttpVersion(0)
{
}

int CHttpPluginRequestMsg::Decode(char *pBuff, int nLen)
{
	int nBodyLen=0;
	char szHead[64]={0};
	char szHeadValue[128]={0};

    char *pBegin=pBuff;
    char *pEnd=strstr(pBegin,"\r\n");
    if(pEnd==NULL||pEnd-pBuff>=nLen) return -1;
        
    *pEnd='\0';
    pBegin=pEnd+2;
	
    char szCommand[256]={0};
    char szAttribute[2048]={0};
	char szHttpVersion[16] = {0};
    if(0==strncmp(pBuff, "GET", 3))
    {
        int nResult=sscanf(pBuff,"GET %255[^?]?%2047s%15s",szCommand,szAttribute,szHttpVersion);
        if(nResult==1)
        {
            nResult=sscanf(pBuff,"GET %255s%15s",szCommand,szHttpVersion);
        }
        if(nResult<2)
        {
            return -1;
        }
        if(strncasecmp(szHttpVersion, "http/1.1", 8) == 0)
		{
			m_nHttpVersion = 1;
			m_isKeepAlive = true;
		}else
		{
			m_nHttpVersion = 0;
			m_isKeepAlive = false;
		}
		
		m_strPath=szCommand;
        m_strBody=szAttribute;
      
        pEnd=strstr(pBegin,"\r\n");
        while(pEnd!=NULL && pBegin-pBuff<nLen && (pEnd+2-pBuff)<nLen)
        {
            *pEnd='\0';            

            if(sscanf(pBegin,"%63[^:]:%127s",szHead,szHeadValue)!=2)
			{				
				pBegin = pEnd+2;
				pEnd = strstr(pBegin,"\r\n");
				continue;
            }
            
            if((strncasecmp(szHead,"connection",10)==0))
			{
				 if(strncasecmp(szHeadValue,"close",6)==0)
				 {
				 	m_isKeepAlive = false;
				 }
				 else if(strncasecmp(szHeadValue,"keep-alive",10)==0)
				 {
				 	m_isKeepAlive = true;
				 }				
			}
			else if(strncasecmp(szHead, "x-forwarded-for", 16) == 0)
			{
				char szIp[128] = {0};
				char *pLocate = strchr(szHeadValue, ',');
				if(NULL != pLocate)
				{	
					memcpy(szIp, szHeadValue, (int)(pLocate-szHeadValue));
				}		
				else		
				{		
					memcpy(szIp, szHeadValue, strlen(szHeadValue));		
				}
				m_strXForwardedFor = szIp;
				m_isXForwardedFor = true;
			}
			
			pBegin=pEnd+2;
            pEnd = strstr(pBegin,"\r\n");
			
        }
    }
    else if(0==strncmp(pBuff, "POST", 4))
    {
        int nResult=sscanf(pBuff,"POST %255s%15s",szCommand,szHttpVersion);
        if(nResult!=2)
        {
            return -1;
        }
		m_strPath=szCommand;
        if(strncasecmp(szHttpVersion, "http/1.1", 8) == 0)
		{
			m_nHttpVersion = 1;
			m_isKeepAlive = true;
		}else
		{
			m_nHttpVersion = 0;
			m_isKeepAlive = false;
		}
        pEnd=strstr(pBegin,"\r\n");
        while(pEnd!=NULL && pBegin-pBuff<nLen && (pEnd+2-pBuff)<nLen)
        {
            *pEnd='\0';            

            if(sscanf(pBegin,"%63[^:]:%127s",szHead,szHeadValue)!=2)
			{
				pBegin = pEnd + 2;
				pEnd = strstr(pBegin, "\r\n");
				continue;
            }
            
            if((strncasecmp(szHead,"connection",10)==0))
			{
				if(strncasecmp(szHeadValue,"close",6)==0)
				{
					m_isKeepAlive = false;
				}
				else if(strncasecmp(szHeadValue,"keep-alive",10)==0)
				{
				 	m_isKeepAlive = true;
				}
				
			}
			else if(strncasecmp(szHead, "x-forwarded-for", 16) == 0)
			{
				char szIp[256] = {0};
				char *pLocate = strchr(szHeadValue, ',');
				if(NULL != pLocate)
				{	
					memcpy(szIp, szHeadValue, (int)(pLocate-szHeadValue));
				}		
				else		
				{		
					memcpy(szIp, szHeadValue, strlen(szHeadValue));		
				}
				m_strXForwardedFor = szIp;
				m_isXForwardedFor = true;
			}
            else if(strncasecmp(szHead,"content-length",14)==0)
			{
				nBodyLen=atoi(szHeadValue);
			}
			pBegin=pEnd+2;
            pEnd = strstr(pBegin,"\r\n");
			
        }
        if (nBodyLen>0) 
		{
			m_strBody = string(pBuff+nLen-nBodyLen,nBodyLen);
			log_debug("CHttpAgentRequestMsg::%s,m_strBody[%s]\n",__FUNCTION__,m_strBody.c_str());
		}
    }
	else
	{
		return -1;
	}
    return 0;
}

string CHttpPluginRequestMsg::GetCommand() const
{
	if(m_strPath.length() > 0 && m_strPath.substr(0,1) == "/")
	{
		return m_strPath.substr(1);
	}
    else
	{
        return m_strPath;
	}
}

string CHttpPluginRequestMsg::GetBody() const
{
	log_debug("CHttpAgentRequestMsg::%s,m_strBody[%s]\n",__FUNCTION__,m_strBody.c_str());
	return m_strBody;
}

int CHttpPluginRequestMsg::HandleHeader(void *pPkg, int iBytesRecved, int *piPkgLen)
{
	char *alloc = new char[iBytesRecved + 1];
	char *header = alloc;
	memcpy(header, pPkg, iBytesRecved);
	header[iBytesRecved] = 0;

	*piPkgLen = 0;
	char *pEnd = strstr((char *)header,"\r\n\r\n");
	while (pEnd != NULL)
	{
		if(strcasestr((char *)header, "HTTP"))
		{
			 char *pFound = strstr((char *)header,"\r\n");
			 int  nBodyLen = -1;
			 char szHead[64]={0};
			 char szHeadValue[512]={0};
			 bool bHaveBody = false;


			 while( pFound != NULL && pFound < pEnd )
			 {
				if(sscanf(pFound+2,"%63[^:]: %511[^\r\n]",szHead,szHeadValue)<2)
				{
					pFound = strstr(pFound+2,"\r\n");
					continue;
				}

				if(0==strncasecmp(szHead,"Content-Length",14))
				{
					nBodyLen = atoi(szHeadValue);
					bHaveBody = true;
					log_info("content-length:%d\n",nBodyLen);
					break;
				}
				else if((0==strncasecmp(szHead,"Transfer-Encoding",17))&&(0==strncasecmp(szHeadValue,"chunked",7)))
				{
					char *pBodyEnd = strstr(pEnd+4, "\r\n0\r\n\r\n");
					if(pBodyEnd != NULL)
					{
						nBodyLen = pBodyEnd - pEnd + 3;  //3=7-4
					}
					bHaveBody = true;
					log_info("chunked packet pakaet len:%d\n", nBodyLen);
					break;
				}

				pFound = strstr(pFound+2,"\r\n");
			}

			if(bHaveBody == false)
			{
				*piPkgLen = pEnd + 4 - (char *)header;
				header = (char *)pEnd + 4;
			}
			else
			{
				if(nBodyLen > 0)
				{
					*piPkgLen = pEnd + 4  +nBodyLen - (char *)header;
					header = pEnd + 4 + nBodyLen;
				}
				else
				{
					break;
				}
			}
		}
		else {
			return -1;
		}
		pEnd = strstr((char *)header,"\r\n\r\n");
	}

	if (alloc)
		delete[] alloc;
	return 0;
}

