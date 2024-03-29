#ifndef __DTC_UTILS__
#define __DTC_UTILS__
#include <vector>
#include <string>

#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h> 

/*此文件放置dtc的工具函数*/
namespace dtc
{
namespace utils
{

	/*************************************************
		获取本机的ip tomchen
	**************************************************/
	inline std::string GetLocalIP()
	{
		int iClientSockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(iClientSockfd < 0 )
		return "0.0.0.0";

		struct sockaddr_in stINETAddr;
		stINETAddr.sin_addr.s_addr = inet_addr("192.168.0.1");
		stINETAddr.sin_family = AF_INET;
		stINETAddr.sin_port = htons(8888);

		int iCurrentFlag = fcntl(iClientSockfd, F_GETFL, 0);
		fcntl(iClientSockfd, F_SETFL, iCurrentFlag | FNDELAY);

		if(connect(iClientSockfd, (struct sockaddr *)&stINETAddr, sizeof(sockaddr)) != 0)
		{
			close(iClientSockfd);
			return "0.0.0.0";
		}

		struct sockaddr_in stINETAddrLocal;
		socklen_t iAddrLenLocal = sizeof(stINETAddrLocal);
		getsockname(iClientSockfd, (struct sockaddr *)&stINETAddrLocal, &iAddrLenLocal);

		close(iClientSockfd);

		return inet_ntoa(stINETAddrLocal.sin_addr);
	}
	/*************************************************
		切割字符串strOri, 以_Ch为分隔符，结果为theVec
	**************************************************/
	inline void SplitStr(std::string strOri, char _Ch, std::vector<std::string>& theVec)
	{
		std::string::size_type nLastPos = 0;		
		std::string strSub;
		std::string::size_type iIndex = strOri.find(_Ch);

		if (std::string::npos == iIndex)
		{
			theVec.push_back(strOri);
			return;
		}

		while( std::string::npos != iIndex)
		{
			strSub = strOri.substr(nLastPos, iIndex - nLastPos);
			nLastPos = iIndex + 1;
			iIndex = strOri.find(_Ch, nLastPos);	
			theVec.push_back(strSub);
		}

		if (nLastPos != 0)
		{
			strSub = strOri.substr(nLastPos, strOri.length() - nLastPos);	
			theVec.push_back(strSub);
		}
	}

}
}

#endif