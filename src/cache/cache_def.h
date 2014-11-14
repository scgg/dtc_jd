#ifndef __CACHE_DEF_H
#define __CACHE_DEF_H

#define E_OK			0				//success
#define E_FAIL			-1				//fail
#define KEY_LEN_LEN		sizeof(char)	//"key��"�ֶγ���
#define MAX_KEY_LEN		256				//key��󳤶�,��"key��"�ֶγ������ܱ�ʾ��������־���
#define ERR_MSG_LEN 	1024
#define MAX_PURGE_NUM	1000			//ÿ��purge�Ľڵ�������
#define CACHE_SVC		"ttc"			//cache������
//#define VERSION			"1.0.3"			//�汾��Ϣ

#define STRNCPY(dest,src,len)	{memset(dest,0x00,len); strncpy(dest,src,len-1); }
#define SNPRINTF(dest,len,fmt,args...)	{ memset(dest,0x00,len); snprintf(dest,len-1,fmt,##args); }
#define MSGNCPY(dest,len,fmt,args...)	{ memset(dest,0x00,len); snprintf(dest,len-1,"[%s][%d]%s: " fmt "\n",__FILE__, __LINE__, __FUNCTION__,##args);}

#endif
