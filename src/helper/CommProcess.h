#ifndef __COMM_PROCESS_H__
#define __COMM_PROCESS_H__

#include <value.h>
#include <field.h>
#include <namespace.h>
#include "HelperLogApi.h"

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

/* helper����Ļ��� 
ʹ������Ҫ�̳�����࣬��ʵ������������GobalInit()��Init()��Execute()��ProcessGet()��virtual�����ӿڡ�
*/
class CCommHelper{
protected:
	int GroupID;
	int Role;
	int Timeout;
	
public:
	// ���캯���������Ҫ���������server�����ӣ������ڹ��캯�����ӡ���Ҫ��Init����Execute��ִ�С�
	CCommHelper();
	virtual ~CCommHelper();

	// ������Ҫʵ�ֵ�3������
	/* helper��һ�������顣���������fork֮ǰ���õ� */
	virtual int GobalInit(void);
	
	/* ���������fork֮����õ� */
	virtual int Init(void);	
	
	/* ÿ���������һ������������� */
	virtual int Execute();
	
	/* ����һ������ĳ�ʱʱ�� ����λΪ��*/
	void SetProcTimeout(int timeout){ Timeout = timeout; }

	friend class CHelperMain;

protected:
	/* ����Get����Ľӿں��� */
	virtual int ProcessGet()=0;
	
	/* ����Insert����ӿں��� */
	virtual int ProcessInsert()=0;
	
	/* ����Delete����ӿں��� */
	virtual int ProcessDelete()=0;
	
	/* ����Update����ӿں��� */
	virtual int ProcessUpdate()=0;
	
	/* ����Replace����ӿں��� */
	virtual int ProcessReplace()=0;


	/* ��ѯ��������ַ */
	const char *GetServerAddress(void) const;

	/* ��ѯ�����ļ� */
	int GetIntVal(const char *sec, const char *key, int def=0);
	unsigned long long GetSizeVal(const char *sec, const char *key, unsigned long long def=0, char unit=0);
	int GetIdxVal(const char *, const char *, const char * const *, int=0);
	const char *GetStrVal (const char *sec, const char *key);
	bool HasSection(const char *sec);
	bool HasKey(const char *sec, const char *key);

	/* ��ѯ���� */
	CTableDefinition* Table(void) const;
	int FieldType(int n) const;
	int FieldSize(int n) const;
	int FieldId(const char *n) const;
	const char* FieldName(int id) const;
	int NumFields(void) const;
	int KeyFields(void) const;
	
	/* ��ȡ�����ͷ��Ϣ */
	const CPacketHeader* Header() const;
	
	/* ��ȡ�����������Ϣ */
	const CVersionInfo* VersionInfo() const;
	
	/* ���������� */
	int RequestCode() const;
	
	/* �����Ƿ���key��ֵ��������key��insert����û��keyֵ�� */
	int HasRequestKey() const;
	
	/* �����keyֵ */
	const CValue *RequestKey() const;
	
	/* ����key��ֵ */
	unsigned int IntKey() const;
	
	/* �����where���� */
	const CFieldValue *RequestCondition() const;
	
	/* update����insert����replace����ĸ�����Ϣ */
	const CFieldValue *RequestOperation() const;
	
	/* get����select���ֶ��б� */
	const CFieldSet *RequestFields() const;
	
	/* ���ô�����Ϣ�������롢�������ط�����ϸ������Ϣ */
	void SetError(int err, const char *from, const char *msg);
	
	/* �Ƿ�ֻ��select count(*)��������Ҫ���ؽ���� */
	int CountOnly(void) const;
	
	/* �Ƿ�û��where���� */
	int AllRows(void) const;
	
	/* �����������еĸ�������row��ͨ��������������rowһһ��������ӿڸ��£�Ȼ�����±�������ݲ㡣������Լ�����RequestOperation���£�����Ҫ������ӿڣ�*/
	int UpdateRow(CRowValue &row);
	
	/* �Ƚ����е���row���Ƿ�������������������ָ��ֻ�Ƚ�ǰ��n���ֶ� */
	int CompareRow(const CRowValue &row, int iCmpFirstNField=256) const ;

	/* ������������һ������ǰ�������ȵ�������ӿڳ�ʼ�� */
	int PrepareResult(void);
	
	/* ��key��ֵ���Ƶ�r */
	void UpdateKey(CRowValue *r) ;

	/* ��key��ֵ���Ƶ�r */
	void UpdateKey(CRowValue &r) ;
	
	/* ����������������select * from table limit m,n������total-rowsһ��Ƚ������������ */
	int SetTotalRows(unsigned int nr);
	
	/* ����ʵ�ʸ��µ����� */
	int SetAffectedRows(unsigned int nr);
	
	/* ����������һ������ */
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
