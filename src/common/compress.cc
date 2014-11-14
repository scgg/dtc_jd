/*
 * =====================================================================================
 *
 *       Filename:  compress.cc
 *
 *    Description:  add for field compress
 *
 *        Version:  1.0
 *        Created:  05/31/2010 01:19:28 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "compress.h"
#include "zlib.h"
#include "memcheck.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>

CCompress::CCompress()
{
    _level = 1;
    _buflen = 0;
    _len = 0;
    _buf = NULL;
}

CCompress::~CCompress()
{
    if (_buf)
        FREE(_buf);
}

void CCompress::SetCompressLevel(int level)
{
    _level = level<1? 1:(level>9? 9:level);
}
int CCompress::SetBufferLen(unsigned long len)
{
    if (_buf==NULL)
    {
        _buflen = len;
        _buf = (unsigned char *)MALLOC(len);
    }
    else if(_buflen<len)
    {
        _buflen = len;
        FREE(_buf);
        _buf = (unsigned char *)MALLOC(len);
    }
    if (_buf==NULL)
        return -ENOMEM;
    return 0;
}

//source 被压缩的缓冲区 sourcelen 被压缩缓冲区的原始长度
//dest   压缩后的缓冲区 destlen   被压缩后的缓冲区长度
//注意调用该函数时， destlen 首先要设置为dest缓冲区最大可以容纳的长度
int  CCompress::Compress(const char *source, unsigned long sourceLen)
{
    if(_buf == NULL || source == NULL )
    {
        return -111111;
    }
    _len = _buflen;
    return compress2(_buf, &_len, (Bytef*)source, sourceLen, _level);
}

//source 待解压的缓冲区 sourcelen 待解压缓冲区的原始长度
//dest   解压后的缓冲区 destlen   解缩后的缓冲区长度
//注意调用该函数时， destlen 首先要设置为dest缓冲区最大可以容纳的长度
int  CCompress::UnCompress(char **buf,int *lenp,const char *source, unsigned long sourceLen)
{
    if(_buf == NULL || source == NULL )
    {
        snprintf(_errmsg,sizeof(_errmsg),"input buffer or uncompress buffer is null");
        return -111111;
    }
    _len = _buflen;
    int iret = uncompress(_buf, &_len, (Bytef*)source, sourceLen);
    if (iret)
    {
        snprintf(_errmsg,sizeof(_errmsg),"uncompress error,error code is:%d.check it in /usr/include/zlib.h",iret);
        return -111111;
    }
    *buf = (char *)MALLOC(_len);
    if (*buf==NULL)
        return -ENOMEM;
    memcpy(*buf,_buf,_len);
    *lenp = _len;
    return 0;
}


