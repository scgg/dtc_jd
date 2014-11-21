#ifndef PROBER_CMD_H
#define PROBER_CMD_H

#include <string>
#include <map>
#include "json/json.h"
#include "prober_result_set.h"
#include "StatClient.h"
#include "cache_memory_info.h"

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

#define PROBER_NO_EMPTY 1

struct ClassInfo;

class CHttpSession;

class CProberCmd
{
public:
	CProberCmd();
	virtual ~CProberCmd();
	/*�麯�������������ʵ��*/
	virtual bool ProcessCmd(Json::Value &in, ProberResultSet &prs);
	bool InitStatInfo(Json::Value &in, ProberResultSet &prs);
	bool InitMemInfo(Json::Value &in, ProberResultSet &prs);
	bool VersionCheck(ProberResultSet &prs);
	bool AsyncCheck(ProberResultSet &prs);
	virtual bool InitAndCheck(Json::Value &in, ProberResultSet &prs);
	virtual int GetSize();
	virtual void Combine(CProberCmd *cmd);
	virtual CHttpSession* GetSession() {
		return m_OwnerSession;
	}
	void SetSession(CHttpSession* s) {
		m_OwnerSession = s;
	}
	virtual bool ReplyBody(std::string &body) {
		return true;
	}
	virtual void Cal();
	int MinNodeId() {
		return m_mem->MinValidNodeID();
	}
	int MaxNodeId() {
		return m_mem->MaxNodeID();
	}
	void AttachMem(CProberCmd *cmd);
	void DetachMem();
	void SetInterval(int start, int end);
	void SetNoEmpty();
	

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
	int m_start, m_end;
	bool m_onlyDirty;
	bool m_noEmpty;
	CHttpSession *m_OwnerSession;
	

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


