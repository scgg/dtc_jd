#include "log.h"
#include "dtcpressure.h"
#include "dtc_t_dtc_example.h"
#include <sstream>

using namespace DTC;
using namespace std;

extern int ikey;
extern int iOpType;
extern int iAPILogLevel;

const int GET_OP = 0;
const int INSERT_OP = 1;
const int UPDATE_OP = 2;
const int DELETE_OP = 3;

/*每次key自动加一*/
CMutex kenMutex;
static int iKeyBegin = 0;
extern int iKeyOp;

static int GetTestKey(){
	/*固定key*/
	if (0 == iKeyOp){
		return ikey;
	}
	/*递增key*/
	else{
		do{
			CScopedLock scopedLock(kenMutex);
			iKeyBegin++;
		}while(false);
		int key = ikey + iKeyBegin;
		return key;
	}
}

/*修改该函数，即可实现不同需求的GET操作测试*/
static int DtcGetTest(DTCt_dtc_example& dtc){
	int rst = 0;

	stringstream sstmp;
	string key;
	sstmp.str("");
	sstmp << ikey;
	sstmp >> key;
	vector<Field> resultSet;
	vector<Field> need;
	if(dtc.get(key, need, resultSet)){
		/*
		printf("---------insert----------\n");
		printf("dtc execute get success!\n");
		printf("---------insert end------\n");
		for(vector<Field>::iterator it = resultSet.begin(); it != resultSet.end(); ++it){
			Field fd = *it;
			printf("---------get----------\n");
			printf("fieldname: %s, fieldtype: %d, fieldvalue: %s\n", fd.fieldName.c_str(), fd.fieldType, fd.fieldValue.c_str());
			printf("---------get end------\n");
		}
		*/
	}
	else{
		printf("dtc execute Get key[%d] , error from :%s,  error msg: %s\n", ikey, dtc.errFrom.c_str(), dtc.errMsg.c_str());
		return rst;
	}
	
	return rst;
}

/*修改该函数，即可实现不同需求的INSERT操作测试*/
static int DtcInsertTest(DTCt_dtc_example& dtc)
{
	int rst = 0;

	stringstream sstmp;
	string key;
	sstmp.str("");
	sstmp << ikey;
	sstmp >> key;
	vector<Field> fieldList;
	Field uid;
	uid.fieldName = "uid";
	uid.fieldType = 2;
	uid.fieldValue = key;
	fieldList.push_back(uid);

	Field name;
	name.fieldName = "name";
	name.fieldType = 4;
	name.fieldValue = "xxxxxxxx";
	fieldList.push_back(name);

	Field city;
	city.fieldName = "city";
	city.fieldType = 4;
	city.fieldValue = "yyyyyy";
	fieldList.push_back(city);

	Field descr;
	descr.fieldName = "descr";
	descr.fieldType = 5;
	descr.fieldValue = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
	fieldList.push_back(descr);

	Field age;
	age.fieldName = "age";
	age.fieldType = 2;
	age.fieldValue = "20";
	fieldList.push_back(age);

	Field salary;
	salary.fieldName = "salary";
	salary.fieldType = 2;
	salary.fieldValue = "4000";
	fieldList.push_back(salary);

	if(dtc.insert(key, fieldList)){
		printf("---------insert----------\n");
		printf("dtc execute insert success!\n");
		printf("---------insert end------\n");
	}else{
		printf("dtc execute insert  key[%d]  , error from %s, error msg %s\n", ikey, dtc.errFrom.c_str(), dtc.errMsg.c_str());
	}

	return rst;
}

/*修改该函数，即可实现不同需求的Update操作测试*/
static int DtcUpdateTest(DTCt_dtc_example& dtc)
{
	int rst = 0;
	
	stringstream sstmp;
	string key;
	sstmp.str("");
	sstmp << ikey;
	sstmp >> key;
	vector<Field> fieldList;

	Field uid;
	uid.fieldName = "uid";
	uid.fieldType = 2;
	uid.fieldValue = key;
	fieldList.push_back(uid);

	Field name;
	name.fieldName = "name";
	name.fieldType = 4;
	name.fieldValue = "xxxxxxxx1";
	fieldList.push_back(name);

	Field city;
	city.fieldName = "city";
	city.fieldType = 4;
	city.fieldValue = "yyyyyy1";
	fieldList.push_back(city);

	Field descr;
	descr.fieldName = "descr";
	descr.fieldType = 5;
	descr.fieldValue = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz1";
	fieldList.push_back(descr);

	Field age;
	age.fieldName = "age";
	age.fieldType = 2;
	age.fieldValue = "40";
	fieldList.push_back(age);

	Field salary;
	salary.fieldName = "salary";
	salary.fieldType = 2;
	salary.fieldValue = "8000";
	fieldList.push_back(salary);

	if(dtc.update(key, fieldList)){
		printf("---------update----------\n");
		printf("dtc execute update success!\n");
		printf("---------update end------\n");
	}else{
		printf("dtc execute update  key[%d], error from %s, error msg %s\n", ikey, dtc.errFrom.c_str(), dtc.errMsg.c_str());
		return rst;
	}

	return rst;
}

/*修改该函数，即可实现不同需求的Delete操作测试*/
static int DtcDeleteTest(DTCt_dtc_example& dtc)
{
	int rst = 0;

	stringstream sstmp;
	string key;
	sstmp.str("");
	sstmp << ikey;
	sstmp >> key;
	if(dtc.delet(key)){
		printf("---------delete----------\n");
		printf("dtc execute delete success!\n");
		printf("---------delete end------\n");
	}else{
		printf("dtc execute delete  key[%d], error from %s, error msg %s\n", ikey, dtc.errFrom.c_str(), dtc.errMsg.c_str());
		return rst;
	}

	return rst;
}

int DtcOpTest(DTCt_dtc_example& dtc){
	if(GET_OP == iOpType){
		return DtcGetTest(dtc);
	}else if(INSERT_OP == iOpType){
		return DtcInsertTest(dtc);
	}else if (UPDATE_OP == iOpType){
		return DtcUpdateTest(dtc);
	}else if (DELETE_OP == iOpType){
		return DtcDeleteTest(dtc);
	}
	
	fprintf(stderr, "unkonw optype %d\n", iOpType);
	return -1;
	
}
class DTCTest : public DTCPresssure::IPressure
{
public:
	DTCTest(){
		m_dtc = NULL;
		DTC_PRESSURE_CTRL->setPressure(static_cast<IPressure *>(this));
	}
	
	~DTCTest()
	{
		if (NULL != m_dtc)
		{
			
		}
	}
	//改函数主线程调用，用于通知测试方进行资源分配
	virtual void PreparePressure(int iThreadTotal, int iIndex, std::string &sName){
		printf("PreparePressure\n");
		TTC::init_log("cplusTplPreTest");
		TTC::set_log_level(iAPILogLevel);
		
		if (NULL == m_dtc){
			m_dtc = new DTCt_dtc_example[iThreadTotal];
		}
		
		if (GET_OP == iOpType){
			sName = "DTC_GET_TEST";
		}

		else if (INSERT_OP == iOpType){
			sName = "DTC_INSERT_TEST";
		}

		else if (UPDATE_OP == iOpType){
			sName = "DTC_UPDATE_TEST";
		}
		 
		else if (DELETE_OP == iOpType){
			sName = "DTC_DELETE_TEST";
		}
		
	}
	//改函数会被对线程调用
	virtual int Pressure(int iThreadId)
	{
		int rst = DtcOpTest(m_dtc[iThreadId]);
		return rst;
	}
	//改函数主线程调用，用于通知测试方进行资源回收
	virtual void FinishPressure()
	{
		printf("finish\n");
	}
private:
	DTC::DTCt_dtc_example* m_dtc;
	int    m_iIndex;
	
};

DTC_PRESS_INSTANCE_DECLARE(DTCTest)

