#ifndef __CHC_CLI_H
#define __CHC_CLI_H

#include "sockaddr.h"
#include <string>
#include <stdint.h>
#include "atomicwindows.h"
#include "tabledef.h"
#include "value.h"
#include "keylist.h"
#include "field_api.h"
#include "task_base.h"
#include "compress.h"
#include "packet.h"

using namespace std;
class CTask;
class CPacket;

class NCResult;
class NCResultInternal;
class NCPool;
struct NCTransation;
class ITTCTaskExecutor;
class ITTCService;

#define DELETE(pointer) \
	do \
{ \
	if(pointer) { \
	delete pointer; \
	pointer = 0; \
	} \
} while(0)

#define NEW(type, pointer) \
	do \
{ \
	try \
{ \
	pointer = 0; \
	pointer = new type; \
} \
	catch (...) \
{ \
	pointer = 0; \
} \
} while (0)



class NCBase {
private:
	TAtomicU32 _count;
public:
	NCBase(void) {}
	~NCBase(void) {}
	int INC(void) { 
		int tmp= ++_count; 

		return tmp;}
	int DEC(void) {
		int volatile tmp = --_count;
		return tmp; }
	int CNT(void) { return _count.get(); };
};


class NCRequest;
class NCServer: public NCBase {
public:	// global server state
	NCServer();

	~NCServer(void);

	CSocketAddress addr;
	
	char *tablename;
	char *appname;

	int keytype;
	int autoUpdateTable;
	int autoReconnect;
	NCKeyInfo keyinfo;

	//key 类型
	int IntKey(void);
	int StringKey(void);
	
	//设置表名
	int SetTableName(const char *);

	//设置地址、端口
	int SetAddress(const char *h, const char *p=NULL);

	// error state
	unsigned completed:1;
	unsigned badkey:1;
	unsigned badname:1;
	unsigned autoping:1;
	const char *errstr;

	const char *ErrorMessage(void) const { return errstr; }

	void Close(void);

	// table definition
	CTableDefinition *tdef;
	CTableDefinition *admin_tdef;


	/*date:2014/06/04, author:xuxinxin*/
	string accessToken;
	int SetAccessKey(const char *token);

	int IsCompleted(void) const { return completed; }

	int IsDgram(void) const { return addr.SocketType()==SOCK_DGRAM; }

private: // serialNr manupulation
	uint64_t lastSN;
public:
	uint64_t NextSerialNr(void) { ++lastSN; if(lastSN==0) lastSN++; return lastSN; }
	uint64_t LastSerialNr(void) { return lastSN; }
	int SimpleBatchKey(void) const { return keyinfo.KeyFields()==1 && keytype == keyinfo.KeyType(0); }
private:
		int netfd;
		void CheckInternalService(void);


// timeout settings
private: 
	int timeout;
	int realtmo;
public:
	void SetMTimeout(int n);
	int GetTimeout(void) const { return timeout; }
	//ping请求，包含很多任务
	int Ping(void);

	//连接请求
	int Connect(void);

	//发送打包数据
	int SendPacketStream(CPacket &);
	//解包结果
	int DecodeResultStream(NCResult &);

private:
	void UpdateTimeout(void);


private:
	ITTCTaskExecutor    *executor;
public:
	NCPool *ownerPool;
	void SaveDefinition(NCResult *);

public:
	int AllowBatchKey(void) const { return keyinfo.KeyFields(); }

};


class NCRequest {
public:
	NCServer *server;
	
	CTableDefinition *tdef;
	char *tablename;
	int keytype;

	uint8_t cmd;
	uint8_t haskey;
	uint8_t flags;
	int err;
	CValue key;
	NCKeyValueList kvl;
	CFieldValueByName ui;
	CFieldValueByName ci;
	CFieldSetByName fs;

	unsigned int limitStart;
	unsigned int limitCount;
	int adminCode;
	uint64_t hotBackupID;
	uint64_t MasterHBTimestamp;
	uint64_t SlaveHBTimestamp;
public:
	NCRequest(NCServer *, int op);
	~NCRequest(void);
	int SetKey(int64_t);
	int SetKey(const char *, int);
	int UnsetKey(void);
	int UnsetKeyValue(void);
	int Need(const char *n, int);	
	NCResult *Execute(const CValue *key=NULL);
	NCResult *Execute(int64_t);
	NCResult *Execute(const char *, int);	


	NCResult *PreCheck(const CValue *key); // return error result, NULL==success
	NCResult *ExecuteNetwork(const CValue *key=NULL);
	NCResult *ExecuteStream(const CValue *key=NULL);
	int Encode(const CValue *key, CPacket *);
	int SetTabDef(void);
public:
	int AddCondition(const char *n, uint8_t op, uint8_t t, const CValue &v);
	int AddOperation(const char *n, uint8_t op, uint8_t t, const CValue &v);

	int AddKeyValue(const char* name, const CValue &v, uint8_t type);


	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, const CValue *key=NULL);
	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, int64_t);
	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, const char *, int);
	int SetCacheID(int dummy) { return err = -EINVAL; }


private:
    uint64_t compressFlag;//field flag

	int initCompress(void);
	char _errmsg[1024];
};



class NCResultLocal {
public:
	const uint8_t *vidmap;
	long long apiTag;
	int maxvid;
	CCompress *gzip;

public:
	NCResultLocal(CTableDefinition* tdef) :
		vidmap(NULL),
		apiTag(0),
		maxvid(0),
		gzip (NULL),
		_tdef(tdef),
		compressid(-1)
	{
	}

	virtual ~NCResultLocal(void) {
		FREE_CLEAR(vidmap);
		DELETE (gzip);
	}

	virtual int FieldIdVirtual(int id) const {
		return id > 0 && id <= maxvid ? vidmap[id-1] : 0;
	}

	virtual void SetApiTag(long long t) { apiTag = t; }
	virtual long long GetApiTag(void) const { return apiTag; }

	void SetVirtualMap(CFieldSetByName &fs)
	{
		if(fs.MaxVirtualId()){
			fs.Resolve(_tdef, 0);
			vidmap = fs.VirtualMap();
			maxvid = fs.MaxVirtualId();
		}
	}

	virtual int initCompress()
	{
		if (NULL == _tdef)
		{
			return -EC_CHECKSUM_MISMATCH;
		}

		int iret = 0;
		compressid = _tdef->CompressFieldId();
		if (compressid<0) return 0;
		if (gzip==NULL)
			NEW(CCompress,gzip);
		if (gzip==NULL)
			return -ENOMEM;
		iret = gzip->SetBufferLen(_tdef->MaxFieldSize());
		if (iret) return iret;
		return 0;
	}

	virtual const int CompressId (void)const {return compressid;}

private:
	CTableDefinition* _tdef;
	uint64_t compressid;

};

class NCResult: public NCResultLocal, public CTask
{
public:
	NCResult(CTableDefinition *tdef) : NCResultLocal(tdef), CTask(tdef, TaskRoleClient, 0) {
		if(tdef) tdef->INC();
		MarkAllowRemoteTable();
	}

	NCResult(int err, const char *from, const char *msg) : NCResultLocal(NULL), CTask(NULL, TaskRoleClient, 1) {
		resultInfo.SetErrorDup(err, from, msg);
	}
	virtual ~NCResult() {
		CTableDefinition *tdef = TableDefinition();
		DEC_DELETE(tdef);
	}



};




#endif