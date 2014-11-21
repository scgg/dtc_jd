/*
 * =====================================================================================
 *
 *       Filename:  network.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/13/2010 05:49:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "network.h"
#include "log.h"
using namespace std;
vector<string> split(const string &str, const char *delimeters)
{
    vector<string> results;
    int pos = 0;
    while(pos != string::npos)
    {
        int next = str.find_first_of(delimeters, pos);
        if(next == string::npos)
        {
            results.push_back(str.substr(pos));
            return results;
        }

        results.push_back(str.substr(pos, next - pos));
        pos = next + 1;
    }

    return results;
}

sockaddr_in string_to_sockaddr(const string &str)
{
	vector<string> parts = split(str, ":");
	sockaddr_in result = { 0 };
	result.sin_family = AF_INET;
	if(parts.size() == 2)
	{
		result.sin_addr.s_addr = inet_addr(parts[0].c_str());
		result.sin_port = htons(atoi(parts[1].c_str()));
	}
	else
	{
		result.sin_addr.s_addr = INADDR_ANY;
		result.sin_port = htons(atoi(parts[0].c_str()));
	}

	return result;
}

int safe_tcp_write(int sock_fd, void* szBuf, int nLen){ 
	int nWrite = 0;   
	int nRet = 0; 
	do { 
		nRet = write(sock_fd, (char*)szBuf + nWrite, nLen - nWrite);   
		if (nRet > 0)    
		{     
			nWrite += nRet;     
			if(nLen == nWrite)    
			{   
				return nWrite;   
			}  
			continue;       
		}    
		else 
		{    
			if (errno == EINTR || errno == EAGAIN)     
			{   
				continue;  
			}     
			else     
			{        
				return nWrite;      
			} 
		}    
	} 
	while(nWrite < nLen);   
	return nLen;
}

int safe_tcp_read(int sock_fd, void* szBuf, int nLen){
	int nRead = 0; 
	int nRet = 0; 
	do {
		nRet = read(sock_fd, (char*)szBuf + nRead, nLen - nRead);
		if (nRet > 0)  
		{
			nRead += nRet;     
			if(nRead == nLen)   
				return nRead;   
			continue;     
		}
		else if (nRet == 0)     
		{     
			return nRead;
		} 
		else    
		{       
			if (errno == EAGAIN || errno == EINTR)    
			{    
				continue;   
			}      
			else     
			{         
				return nRead;     
			}    
		} 
	}while(nRead < nLen);      
	return nLen;
}

void send_result(int sock_fd,const char * buf)
{
		std::string tmp(buf);
		send_result(sock_fd,tmp);
}

void send_result(int sock_fd,string& buf)
{
		int send_len = safe_tcp_write(sock_fd,(void *)buf.c_str(),buf.length());
		if (send_len != buf.length())
		{
				log_error("send result error. send len:%d result len:%d result:%s",
								send_len,buf.length(),buf.c_str());
				close(sock_fd);
		}
		log_debug("%s",buf.c_str());
}


