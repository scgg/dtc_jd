/*
 * =====================================================================================
 *
 *       Filename:  http_recevier.cc
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#include "stream_receiver.h"
#include "log.h"

CStreamReceiver::CStreamReceiver(int f, int s/*header size*/, IReceiver *q):
                        fd(f),
			iBytesRecved(0),
			iPkgLen(0),
			iPkgHeadLen(s),
                        query(q)
{
}

CStreamReceiver::~CStreamReceiver()
{

}

int CStreamReceiver::Recv()
{
	if (query == NULL)
		return -1;
	int iRet = recv(fd, recvBuf + iBytesRecved, sizeof(recvBuf) - iBytesRecved, 0);

	if (iRet < 0)
	{
		return -1;
	}
	//对端关闭了连接
	if (iRet == 0)
	{
		return -1;
	}

	iBytesRecved += iRet;

	for (;;)
	{
		if (iPkgLen == 0)
		{							// 未获取包长度
			if (iBytesRecved < iPkgHeadLen)
			{
				break;				// 未收全包头
			}

			// 获取包长度
			if (query->HandleHeader(recvBuf, iBytesRecved, &iPkgLen) < 0)
			{
				return -1;
			}

			if (iPkgLen < iPkgHeadLen)
			{
				return -1;
			}
		}

		int len = iPkgLen;

		if (len > sizeof(recvBuf))
		{
			return -1;
		}

		if (iBytesRecved < len)
		{
			break;
		}							//未收全整个包

		// 如果HandlePkgHead返回包的大小为0，则直接中断不处理 [3/30/2011 linjinming]
		if (len == 0)
		{
			break;
		}


		// 已收全整个包
		if (query->HandlePkg(recvBuf, len) < 0)
		{
			return -1;
		}
		break;
		/*
		iPkgLen = 0;
		iBytesRecved -= len;
		
		if (iBytesRecved == 0)
		{
			break;
		}

		memmove(recvBuf, recvBuf + len, iBytesRecved);
		*/
	}

	return 0;
}
