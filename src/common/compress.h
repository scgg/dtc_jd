/*
 * =====================================================================================
 *
 *       Filename:  compress.h
 *
 *    Description:  gz
 *
 *        Version:  1.0
 *        Created:  05/31/2010 01:33:27 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
class CCompress
{
    public:
        CCompress();
        virtual ~CCompress();
        void SetCompressLevel(int level);
        int SetBufferLen(unsigned long len);
        const char * ErrorMessage(void) const{return _errmsg;}

        //source 被压缩的缓冲区 sourcelen 被压缩缓冲区的原始长度
        //dest   压缩后的缓冲区 destlen   被压缩后的缓冲区长度
        //注意调用该函数时， destlen 首先要设置为dest缓冲区最大可以容纳的长度
        int  Compress(const char *source, unsigned long sourceLen); 

        //source 待解压的缓冲区 sourcelen 待解压缓冲区的原始长度
        //dest   解压后的缓冲区 destlen   解缩后的缓冲区长度
        //注意调用该函数时， destlen 首先要设置为dest缓冲区最大可以容纳的长度
        int UnCompress(char **dest,int *destlen,const char *source, unsigned long sourceLen);
        
        unsigned long GetLen(void){return _len;}
        char * GetBuf(void){return (char *)_buf;}
        char _errmsg[512];
    private:
        unsigned long _buflen;
        unsigned char * _buf;
        unsigned long _len;
        int  _level;
};

