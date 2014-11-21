/*
 * =====================================================================================
 *
 *       Filename:  http_recevier.h
 *
 *    Description:  tcp stream proactor receiver
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

#ifndef __STREAM_RECEIVER_H__
#define __STREAM_RECEIVER_H__

#include <stdint.h>

class IReceiver
{
public:
	virtual int HandleHeader(void *pPkg, int iBytesRecved, int *piPkgLen) = 0;
	virtual int HandlePkg(void *pPkg, int iPkgLen) = 0;
};

class CStreamReceiver
{
private:
	char       recvBuf[15*1024];
    int        fd;
    int        iBytesRecved;
    int        iPkgLen;
    int        iPkgHeadLen;
    IReceiver *query;
public:
    CStreamReceiver(int f, int s/*header size*/, IReceiver *q);
    ~CStreamReceiver();

    int Recv();
};

#endif
