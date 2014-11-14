#ifndef PROBER_CMD_H
#define PROBER_CMD_H

#include <string>
#include <map>
#include "json/json.h"
#include "ProberResultSet.h"
#include "StatClient.h"
#include "CacheMemoryInfo.h"

#define MEM_CMD_BASE 50000
#define DISTRI_NUM 8

#define PROBER_ITERATE_EMPTY 1
#define PROBER_ITERATE_CLEAN 2
#define PROBER_ITERATE_DIRTY 4

#define PROBER_INTERVAL_ARRAY 4
#define PROBER_INTERVAL_LEFT 0
#define PROBER_INTERVAL_RIGHT 1
#define PROBER_INTERVAL_COUNTER 2
#define PROBER_INTERVAL_TYPE 3
/* LO for left open, LC for left close */
#define PROBER_INTERVAL_TYPE_LO_RO 0
#define PROBER_INTERVAL_TYPE_LC_RO 1
#define PROBER_INTERVAL_TYPE_LO_RC 2
#define PROBER_INTERVAL_TYPE_LC_RC 3

struct ClassInfo;

class CProberCmd
{
public:
	CProberCmd();
	virtual ~CProberCmd();
	/*�麯�������������ʵ��*/
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs) = 0;

	bool InitStatInfo(Json::Value &in, ProberResultSet &prs);
	bool InitMemInfo(Json::Value &in, ProberResultSet &prs);
	bool VersionCheck(ProberResultSet &prs);
	bool AsyncCheck(ProberResultSet &prs);
	bool IterateMem(int flag = PROBER_ITERATE_EMPTY | PROBER_ITERATE_CLEAN | PROBER_ITERATE_DIRTY);
	virtual int GetSize();
	

	/*ע�ắ��*/
	static bool Register(ClassInfo* classInfo);
	/*�����������ʵ��*/
	static CProberCmd* CreatConcreteCmd(const std::string& className);

protected:
	CStatClient m_stc;
	CCacheMemoryInfo *m_mem;
	int m_ver;
	int m_dis[DISTRI_NUM][PROBER_INTERVAL_ARRAY];
	int m_datas;
	int m_empty;
	int m_keys;
	

private:
	/*�������ž������������ƺ�������֮���ӳ���ϵ*/
	static std::map<std::string, ClassInfo*> m_classInfoMap;

};

/*ÿ��������������������ƺͲ�������ʵ���ĺ���ָ��*/
typedef CProberCmd* (*CreatObject)();	
struct ClassInfo
{
	public:
	std::string strClassName;
	CreatObject fun;
	ClassInfo(std::string& className, CreatObject fun):strClassName(className), fun(fun)
	{			
		CProberCmd::Register(this);
	}
	ClassInfo(const char* charName, CreatObject fun):strClassName(charName), fun(fun)
	{			
		CProberCmd::Register(this);
	}

};


/*�����꣬��������ʹ��*/
/*����һ����������Ϣ�ľ�̬����*/
#define DECLARE_CLASS_INFO()\
private:\
	static ClassInfo* m_classInfo;

/*����һ����������Ϣ�ľ�̬�������������֣�������ʵ��ĺ�������ƥ���ϵ,ͬʱ��ClassInfo����ʵ����ʱ���ע��*/
#define DEFINE_REG_CALSS_INFO(ClassName) \
	ClassInfo* ClassName::m_classInfo = new ClassInfo(#ClassName,ClassName::CreatObject);

/*������ʵ��ĺ���*/
#define IMPL_CLASS_OBJECT_CREATE(className) \
	static CProberCmd* CreatObject() \
    {\
		return new className;\
    }

#endif


