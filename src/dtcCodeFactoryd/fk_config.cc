#include "fk_config.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

//去除首尾空格
char* FK_Config::trimTail (char* pStr)
{
	int i;
	
	if(!pStr)
		return pStr;
		
	for (i = strlen(pStr) - 1; i >= 0; i--)
	{
		if (*(pStr + i) != ' ' && *(pStr + i) != '\t' && *(pStr + i) != '\n' && *(pStr + i) != '\r')
			break;
	}
	*(pStr + i + 1) = '\0';

	return (pStr);
}

char* FK_Config::trimHead(char* pStr)
{
	char *p, *q;
	
	if(!pStr)
		return pStr;
	
	p = q = pStr;
	while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
		p++;
	if (p == q)
		return (pStr);
	while (*p)
		*q++ = *p++;
	*q = 0;

	return (pStr);
}

FK_Config::FK_Config()
{
}

FK_Config::~FK_Config()
{
}

//读取配置文件，保存到map
int FK_Config::Init(const char* sConfFilePath, char* sErrMsg)
{
	FILE*	fFile;

	if ((fFile = fopen(sConfFilePath, "r")) == NULL)
	{
		sprintf(sErrMsg, "fail to open file %s", sConfFilePath);
		return -1;
	}

	m_mapConfig.clear();

	char	sLineBuf[512];

	while (fgets(sLineBuf, sizeof(sLineBuf), fFile) != NULL)
	{
		if (sLineBuf[0] == '#')				//忽略注释行
		{
			continue;
		}

		char*	p = strstr(sLineBuf, "#");	//忽略注释符号后的内容
		if (p)
		{
			*p = '\0';
		}

		char*	sKey;
		char*	sValue;
		p = strstr(sLineBuf, "=");			//忽略注释符号后的内容
		if (p)
		{
			*p = '\0';
			sKey = sLineBuf;
			sValue = p + 1;
		}
		else 
		{
			sprintf(sErrMsg, "this line have no");
			continue;
		}
			
		trimTail(sKey);
		trimHead(sKey);
		trimTail(sValue);
		trimHead(sValue);

		string	strKey = sKey;
		string	strValue = sValue;
		m_mapConfig[strKey] = strValue;
	}

	fclose(fFile);
	return 0;
}

int	FK_Config::GetIntValue(const string& strKey, int& iResult)
{
	string	strResult = m_mapConfig[strKey];
	if (strResult == "")
	{
		printf("get %s error\n", strKey.c_str());
		return -1;
	}
	iResult = (int)atof(strResult.c_str());
	
	return 0;
}

int	FK_Config::GetLonglongValue(const string& strKey, long long& iResult)
{
	string	strResult = m_mapConfig[strKey];
	if (strResult == "")
	{
		printf("get %s error\n", strKey.c_str());
		return -1;
	}
	iResult = atoll(strResult.c_str());
	
	return 0;
}

int	FK_Config::GetDoubletValue(const string& strKey, double& iResult)
{
	string	strResult = m_mapConfig[strKey];
	if (strResult == "")
	{
		printf("get %s error\n", strKey.c_str());
		return -1;
	}
	iResult = atof(strResult.c_str());
	
	return 0;
}

int FK_Config::GetStringValue(const string& strKey, string& strResult)
{
	strResult = m_mapConfig[strKey];
	/*if (strResult == "")
	{
		printf("get %s error\n", strKey.c_str());
		return -1;
	}*/
	
	return 0;
}

