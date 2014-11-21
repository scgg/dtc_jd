#ifndef __CHC_CLI_H
#define __CHC_CLI_H

#include <tabledef.h>
#include <field_api.h>
#include <packet.h>
#include <unistd.h>
#include <cache_error.h>
#include <log.h>
#include "keylist.h"
#include <task_request.h>
#include <container.h>
#include <sockaddr.h>
#include "udppool.h"
#include "compress.h"
#include "poll_thread.h"
#include "lock.h"
#include <map>

/*
 * Goal:
 * 	single transation (*)
 * 	batch transation (*)
 * 	async transation
 */

class CTask;
class CPacket;

class NCResult;
class NCResultInternal;
class NCPool;
struct NCTransation;
class ITTCTaskExecutor;
class ITTCService;

class NCBase {
private:
	TAtomicU32 _count;
public:
	NCBase(void) {}
	~NCBase(void) {}
	int INC(void) { return ++_count; }
	int DEC(void) { return --_count; }
	int CNT(void) { return _count.get(); };
};

class NCRequest;
class DataConnector;
class NCServer: public NCBase {
public:	// global server state
	NCServer();
	NCServer(const NCServer &);
	~NCServer(void);

	// base settings
	CSocketAddress addr;
	char *tablename;
	char *appname;

	static DataConnector *dc;

    //for compress
    void SetCompressLevel(int level){compressLevel=level;}
    int GetCompressLevel(void){return compressLevel;}

    //ttc set _server_address and _server_tablename for plugin
    static char * _server_address;
    static char * _server_tablename;

	int keytype;
	int autoUpdateTable;
	int autoReconnect;
    static int _network_mode;
	NCKeyInfo keyinfo;

	void CloneTabDef(const NCServer& source);
	int SetAddress(const char *h, const char *p=NULL);
	int SetTableName(const char *);
	const char * GetAddress(void) const { return addr.Name(); }//this addres is set by user
	const char * GetServerAddress(void) const { return _server_address; }//this address is set by ttc
	const char * GetServerTableName(void) const { return _server_tablename;}
	int IsDgram(void) const { return addr.SocketType()==SOCK_DGRAM; }
	int IsUDP(void) const { return addr.SocketType()==SOCK_DGRAM && addr.SocketFamily()==AF_INET; }
	const char * GetTableName(void) const { return tablename; }
	int IntKey(void);
	int StringKey(void);
	int FieldType(const char*);
	int IsCompleted(void) const { return completed; }
	int AddKey(const char* name, uint8_t type);
	int KeyFieldCnt(void) const { return keyinfo.KeyFields() ? : keytype != DField::None ? 1 : 0; }
	int AllowBatchKey(void) const { return keyinfo.KeyFields(); }
	int SimpleBatchKey(void) const { return keyinfo.KeyFields()==1 && keytype == keyinfo.KeyType(0); }

	void SetAutoUpdateTab(bool autoUpdate){ autoUpdateTable = autoUpdate?1:0; }
	void SetAutoReconnect(int reconnect){ autoReconnect = reconnect; }
	
	// error state
	unsigned completed:1;
	unsigned badkey:1;
	unsigned badname:1;
	unsigned autoping:1;
	const char *errstr;

	const char *ErrorMessage(void) const { return errstr; }

	// table definition
	CTableDefinition *tdef;
	CTableDefinition *admin_tdef;
	
	void SaveDefinition(NCResult *);
	CTableDefinition* GetTabDef(int cmd) const;
	
	/*date:2014/06/04, author:xuxinxin*/
	std::string accessToken;
	int SetAccessKey(const char *token);
private: // serialNr manupulation
	uint64_t lastSN;
public:
	uint64_t NextSerialNr(void) { ++lastSN; if(lastSN==0) lastSN++; return lastSN; }
	uint64_t LastSerialNr(void) { return lastSN; }

private: // timeout settings
	int timeout;
	int realtmo;
public:
	void SetMTimeout(int n);
	int GetTimeout(void) const { return timeout; }
private:
    int compressLevel;

private: // sync execution
	ITTCService         *iservice;
	ITTCTaskExecutor    *executor;
	int                 netfd;
	time_t              lastAct;
	NCRequest           *pingReq;

public:
	int Connect(void);
	int Reconnect(void);
	void Close(void);
	void SetFD(int fd) { Close(); netfd = fd; UpdateTimeout(); }
	// stream, connected
	int SendPacketStream(CPacket &);
	int DecodeResultStream(NCResult &);
	// dgram, connected or connectless
	int SendPacketDgram(CSocketAddress *peer, CPacket &);
	int DecodeResultDgram(CSocketAddress *peer, NCResult &);
	// connectless
	NCUdpPort *GetGlobalPort(void);
	void PutGlobalPort(NCUdpPort *);

	void TryPing(void);
	int Ping(void);
	void AutoPing(void) { if(!IsDgram()) autoping = 1; }
	NCResultInternal *ExecuteInternal(NCRequest &rq, const CValue *kptr) { return executor->TaskExecute(rq, kptr); }
	int HasInternalExecutor(void) const { return executor != 0; }
private:
	int BindTempUnixSocket(void);
	void UpdateTimeout(void);
	// this method is weak, and don't exist in libttc.a
	__attribute__((__weak__)) void CheckInternalService(void);

public: // transation manager, impl at ttcpool.cc
	int AsyncConnect(int &);
	NCPool *ownerPool;
	int ownerId;
	void SetOwner(NCPool *, int);

	NCResult *DecodeBuffer(const char *, int);
	static int CheckPacketSize(const char *, int);
};



class NCRequest {
public:
	NCServer *server;
	uint8_t cmd;
	uint8_t haskey;
	uint8_t flags;
	int err;
	CValue key;
	NCKeyValueList kvl;
	CFieldValueByName ui;
	CFieldValueByName ci;
	CFieldSetByName fs;
	
	CTableDefinition *tdef;
	char *tablename;
	int keytype;

	unsigned int limitStart;
	unsigned int limitCount;
	int adminCode;

	uint64_t hotBackupID;
	uint64_t MasterHBTimestamp;
	uint64_t SlaveHBTimestamp;
	
public:
	NCRequest(NCServer *, int op);
	~NCRequest(void);
	int AttachServer(NCServer *);

	void EnableNoCache(void) { flags |= DRequest::Flag::NoCache; }
	void EnableNoNextServer(void) { flags |= DRequest::Flag::NoNextServer; }
	void EnableNoResult(void) { flags |= DRequest::Flag::NoResult; }
	int AddCondition(const char *n, uint8_t op, uint8_t t, const CValue &v);
	int AddOperation(const char *n, uint8_t op, uint8_t t, const CValue &v);
    int CompressSet(const char *n,const char * v,int len);
    int CompressSetForce(const char *n,const char * v,int len);
	int AddValue(const char *n, uint8_t t, const CValue &v);
	int Need(const char *n, int);
	void Limit(unsigned int st, unsigned int cnt) {
		if(cnt==0) st = 0;
		limitStart = st;
		limitCount = cnt;
	}
	
	int SetKey(int64_t);
	int SetKey(const char *, int);
	int UnsetKey(void);
	int UnsetKeyValue(void);
	int FieldType(const char* name){ return server?server->FieldType(name):DField::None; }
	int AddKeyValue(const char* name, const CValue &v, uint8_t type);
	int SetCacheID(int dummy) { return err = -EINVAL; }
	void SetAdminCode(int code){ adminCode = code; }
	void SetHotBackupID(uint64_t v){ hotBackupID=v; }
	void SetMasterHBTimestamp(uint64_t t){ MasterHBTimestamp=t; }
	void SetSlaveHBTimestamp(uint64_t t){ SlaveHBTimestamp=t; }
	
	// never return NULL
	NCResult *Execute(const CValue *key=NULL);
	NCResult *ExecuteStream(const CValue *key=NULL);
	NCResult *ExecuteDgram(CSocketAddress *peer, const CValue *key = NULL);
	NCResult *ExecuteNetwork(const CValue *key=NULL);
	NCResult *ExecuteInternal(const CValue *key=NULL);
	NCResult *Execute(int64_t);
	NCResult *Execute(const char *, int);
	NCResult *PreCheck(const CValue *key); // return error result, NULL==success
    int SetCompressFieldName(void);//Need compress flag for read,or set compressFlag for write
	int Encode(const CValue *key, CPacket *);
	// return 1 if tdef changed...
	int SetTabDef(void);

	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, const CValue *key=NULL);
	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, int64_t);
	int EncodeBuffer(char *&ptr, int&len, int64_t &magic, const char *, int);
    const char* ErrorMessage(void) const
    {
        return _errmsg;
    }

private:
        int setCompressFlag(const char * name)
        {
            if (tdef==NULL)
                return -EC_NOT_INITIALIZED;
            if (tdef->FieldId(name) >= 64)
            {
                snprintf(_errmsg, sizeof(_errmsg), "compress error:field id must less than 64"); 
                return -EC_COMPRESS_ERROR;
            }
            compressFlag|=(1<<tdef->FieldId(name));
            return 0;
        }
        uint64_t compressFlag;//field flag
        CCompress *gzip;
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

class NCResultInternal: public NCResultLocal, public CTaskRequest
{
	public:
		NCResultInternal(CTableDefinition* tdef=NULL) : NCResultLocal (tdef)
		{
		}

		virtual ~NCResultInternal() 
		{
		}

		static inline int VerifyClass(void)
		{
			NCResultInternal *ir = 0;
			NCResult *er = reinterpret_cast<NCResult *>(ir);

			NCResultLocal *il = (NCResultLocal *) ir;
			NCResultLocal *el = (NCResultLocal *) er;

			CTask *it = (CTask *) ir;
			CTask *et = (CTask *) er;

			long dl = reinterpret_cast<char *>(il) - reinterpret_cast<char *>(el);
			long dt = reinterpret_cast<char *>(it) - reinterpret_cast<char *>(et);
			return dl==0 && dt==0;
		}
};

/*date:2014/06/09, author:xuxinxin 模调上报 */
class DataConnector : public CThread
{
	struct businessStatistics
	{
		uint64_t TotalTime;			// 10s内请求总耗时
		uint32_t TotalRequests;		// 10s内请求总次数

	public:
		businessStatistics(){ TotalTime = 0; TotalRequests = 0; }
	};
	private:
		uint32_t bid;
		std::map<uint32_t, businessStatistics> mapBi;
		CMutex _lock;			// 读写  TotalTime、TotalRequests时，加锁，防止脏数据
		static DataConnector *pDataConnector;
		DataConnector();
		~DataConnector();

	public:
		static DataConnector* getInstance()
		{
			if( pDataConnector == NULL)
				pDataConnector = new DataConnector();
			return pDataConnector;
		};

	public:
		int SendData();
		int SetReportInfo(std::string str, uint64_t t);
		void GetReportInfo(std::map<uint32_t, businessStatistics> &mapData);
		int SetBussinessId(std::string str);
		virtual void *Process(void);
};


#endif
