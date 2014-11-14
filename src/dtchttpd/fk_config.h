#ifndef FK_CONFIG_H
#define FK_CONFIG_H

#include <map>
#include <string>

using namespace std;

class	FK_Config
{
private:
	char*	trimTail(char* String);
	char*	trimHead(char* string);
	map<string, string> m_mapConfig;

public:
	//读取配置文件，保存到map
	FK_Config();
	~FK_Config();
	
	int Init(const char* sConfFilePath, char* sErrMsg);
	int	GetIntValue(const string& strKey, int& iResult);
	int	GetDoubletValue(const string& strKey, double& iResult);
	int	GetLonglongValue(const string& strKey, long long& iResult);
	int	GetStringValue(const string& strKey, string& strResult);
};
#endif

