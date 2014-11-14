#ifndef __COMM_PROCESS_H__
#define __COMM_PROCESS_H__

#include <value.h>
#include <field.h>
#include <namespace.h>
#include <HelperLogApi.h>

TTC_BEGIN_NAMESPACE

class CCommHelper;
class CHelperMain;
class CTableDefinition;
class CDbConfig;
class CRowValue;
class CPacketHeader;
class CVersionInfo;
class CConfig;
union CValue;
class CFieldValue;
class CFieldSet;

typedef CCommHelper* (*CreateHandle)(void);

/* helper插件的基类 
使用者需要继承这个类，并实现析构函数、GobalInit()、Init()、Execute()、ProcessGet()等virtual函数接口。
*/
class CCommHelper{
protected:
	int GroupID;
	int Role;
	int Timeout;
	
public:
	// 构造函数。如果需要建立跟别的server的连接，不能在构造函数连接。需要在Init或者Execute里执行。
	CCommHelper();
	virtual ~CCommHelper();

	// 服务需要实现的3个函数
	/* helper是一个进程组。这个函数是fork之前调用的 */
	virtual int GobalInit(void);
	
	/* 这个函数是fork之后调用的 */
	virtual int Init(void);	
	
	/* 每个请求调用一次这个函数处理 */
	virtual int Execute();
	
	/* 处理一个请求的超时时间 ，单位为秒*/
	void SetProcTimeout(int timeout){ Timeout = timeout; }

	friend class CHelperMain;

protected:
	/* 处理Get请求的接口函数 */
	virtual int ProcessGet()=0;
	
	/* 处理Insert请求接口函数 */
	virtual int ProcessInsert()=0;
	
	/* 处理Delete请求接口函数 */
	virtual int ProcessDelete()=0;
	
	/* 处理Update请求接口函数 */
	virtual int ProcessUpdate()=0;
	
	/* 处理Replace请求接口函数 */
	virtual int ProcessReplace()=0;


	/* 查询服务器地址 */
	const char *GetServerAddress(void) const;

	/* 查询配置文件 */
	int GetIntVal(const char *sec, const char *key, int def=0);
	unsigned long long GetSizeVal(const char *sec, const char *key, unsigned long long def=0, char unit=0);
	int GetIdxVal(const char *, const char *, const char * const *, int=0);
	const char *GetStrVal (const char *sec, const char *key);
	bool HasSection(const char *sec);
	bool HasKey(const char *sec, const char *key);

	/* 查询表定义 */
	CTableDefinition* Table(void) const;
	int FieldType(int n) const;
	int FieldSize(int n) const;
	int FieldId(const char *n) const;
	const char* FieldName(int id) const;
	int NumFields(void) const;
	int KeyFields(void) const;
	
	/* 获取请求包头信息 */
	const CPacketHeader* Header() const;
	
	/* 获取请求包基本信息 */
	const CVersionInfo* VersionInfo() const;
	
	/* 请求命令字 */
	int RequestCode() const;
	
	/* 请求是否有key的值（自增量key，insert请求没有key值） */
	int HasRequestKey() const;
	
	/* 请求的key值 */
	const CValue *RequestKey() const;
	
	/* 整型key的值 */
	unsigned int IntKey() const;
	
	/* 请求的where条件 */
	const CFieldValue *RequestCondition() const;
	
	/* update请求、insert请求、replace请求的更新信息 */
	const CFieldValue *RequestOperation() const;
	
	/* get请求select的字段列表 */
	const CFieldSet *RequestFields() const;
	
	/* 设置错误信息：错误码、错误发生地方、详细错误信息 */
	void SetError(int err, const char *from, const char *msg);
	
	/* 是否只是select count(*)，而不需要返回结果集 */
	int CountOnly(void) const;
	
	/* 是否：没有where条件 */
	int AllRows(void) const;
	
	/* 根据请求已有的更新数据row。通常将符合条件的row一一调用这个接口更新，然后重新保存回数据层。（如果自己根据RequestOperation更新，则不需要用这个接口）*/
	int UpdateRow(CRowValue &row);
	
	/* 比较已有的行row，是否满足请求条件。可以指定只比较前面n个字段 */
	int CompareRow(const CRowValue &row, int iCmpFirstNField=256) const ;

	/* 在往结果集添加一行数据前，必须先调用这个接口初始化 */
	int PrepareResult(void);
	
	/* 将key的值复制到r */
	void UpdateKey(CRowValue *r) ;

	/* 将key的值复制到r */
	void UpdateKey(CRowValue &r) ;
	
	/* 设置总行数（对于select * from table limit m,n的请求，total-rows一般比结果集的行数大） */
	int SetTotalRows(unsigned int nr);
	
	/* 设置实际更新的行数 */
	int SetAffectedRows(unsigned int nr);
	
	/* 往结果集添加一行数据 */
	int AppendRow(const CRowValue &r);
	int AppendRow(const CRowValue *r);

private:
	void *addr;
	long check;
	char _name[16];
	char _title[80];
	int  _titlePrefixSize;
	const char *_server_string;
	const CDbConfig *_dbconfig;
	CConfig *_config;
	CTableDefinition *_tdef;
		
private:
	CCommHelper(CCommHelper &);
	void Attach(void*);
	
	void InitTitle(int group, int role);
	void SetTitle(const char *status);
	const char *Name(void) const{ return _name; }

public:
	CHelperLogApi logapi;
};

TTC_END_NAMESPACE

#endif
