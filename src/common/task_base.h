#ifndef __CH_TASK_H__
#define __CH_TASK_H__

#include "tabledef.h"
#include "section.h"
#include "field.h"
#include "packet.h"
#include "result.h"
#include "cache_error.h"
#include "timestamp.h"
#include <sys/time.h>
#include "log.h"
#include "buffer.h"
#include "receiver.h"

class NCRequest;

enum CDecodeResult {
	DecodeFatalError,	// no response needed
	DecodeDataError,	// response with error code
	DecodeIdle,		// no data received
	DecodeWaitData,		// partial decoding
	DecodeDone		// full packet
};

enum CDecodeStage { // internal use
	DecodeStageFatalError,	// same as result
	DecodeStageDataError,	// same as result
	DecodeStageIdle,	// same as result
	DecodeStageWaitHeader,	// partial header
	DecodeStageWaitData,	// partial data
	DecodeStageDiscard,	// error mode, discard remain data
	DecodeStageDone		// full packet
};

enum CTaskRole {
	TaskRoleServer=0, /* server, for incoming thread */
	TaskRoleClient,   /* client, reply from server */
	TaskRoleHelperReply, /* helper, reply from server */
};

enum CTaskType {
	TaskTypeAdmin = 0,
	TaskTypeRead,    /* Read Only operation */
	TaskTypeWrite,   /* Modify data */
	TaskTypeCommit,  /* commit dirty data */
};

enum CHITFLAG { 
	HIT_INIT = 0,	// hit init flag
	HIT_SUCCESS = 1,	// hit success flag
};


class CTask: public CTableReference {
	public:
		static const CDecodeResult stage2result[];
		CDecodeResult GetDecodeResult(void) {return stage2result[stage];};
		static const uint32_t validcmds[];
		static const uint16_t cmd2type[];
		static const uint16_t validsections[][2];
		static const uint8_t validktype[DField::TotalType][DField::TotalType];
		static const uint8_t validxtype[DField::TotalOperation][DField::TotalType][DField::TotalType];
		static const uint8_t validcomps[DField::TotalType][DField::TotalComparison];

	protected:
		class BufferPool {
			// simple buffer pool, only keep 2 buffers
			public:
				char *ptr[2];
				int len[2];
				BufferPool() {
					ptr[0] = NULL;
					ptr[1] = NULL;
				}
				~BufferPool() {
					FREE_IF(ptr[0]);
					FREE_IF(ptr[1]);
				}
				void Push(char *v) {
					ptr[ptr[0] ? 1 : 0] = v;
				}

				/* newman: pool */
				inline char *Allocate(int size, CTaskRole role) 
				{
				    if(role == TaskRoleServer || role == TaskRoleClient)
				    {
					CreateBuff(size, len[0], &ptr[0]);
					if(ptr[0] == NULL)
					    return NULL;
					return ptr[0];
				    }else
				    {
					CreateBuff(size, len[1], &ptr[1]);
					if(ptr[1] == NULL)
					    return NULL;
					return ptr[1];
				    }
			//		return ptr[ptr[0] ? 1 : 0] = (char *)MALLOC(size);
				}
				char *Clone(char *buff, int size, CTaskRole role) {
					if(role != TaskRoleClient)
					    return NULL;
					char * p;
					if(ptr[0])
					    p = ptr[1] = (char *)MALLOC(size);
					else
					    p = ptr[0] = (char *)MALLOC(size);

					if(p) memcpy(p, buff, size);
					return p;
				}
		};

	public:
		char * migratebuf;
	protected: // decoder informations
		CDecodeStage stage;
		CTaskRole role;
		BufferPool packetbuf;
		
		//CTableDefinition *tableDef;
		//don't use it except packet decoding
		CTableDefinition *dataTableDef;
		CTableDefinition *hotbackupTableDef;

	protected: // packet info, read-only
		CFieldValue *updateInfo;
		CFieldValue *conditionInfo;
		CFieldSet *fieldList;
	public:
		CResultSet *result;

	public: // packet info, read-write
		//CPacketHeader headerInfo;
		CVersionInfo versionInfo;
		CRequestInfo requestInfo;
		CResultInfo resultInfo;

	protected: // working data
		uint64_t serialNr; /* derived from packet */
		const CValue *key; /* derived from packet */
		const CValue *rkey; /* processing */
		/* newman: pool, resultWriter only create once in task entire life */
		CResultWriter *resultWriter; /* processing */
		int resultWriterReseted;
		uint8_t requestCode; /* derived from packet */
		uint8_t requestType; /* derived from packet */
		uint8_t requestFlags; /* derived from packet */
		uint8_t replyCode; /* processing */
		uint8_t replyFlags; /* derived from packet */
		enum {
			PFLAG_REMOTETABLE = 1,
			PFLAG_ALLROWS = 2,
			PFLAG_PASSTHRU = 4,
			PFLAG_ISHIT = 8,
			PFLAG_FETCHDATA = 0x10,
			PFLAG_ALLOWREMOTETABLE = 0x20,
			PFLAG_FIELDSETWITHKEY = 0x40,
			PFLAG_BLACKHOLED = 0x80,
		};
		uint8_t processFlags; /* processing */

	protected:
		// return total packet size
		int DecodeHeader(const CPacketHeader &in, CPacketHeader *out = NULL); 
		int ValidateSection(CPacketHeader &header);
		void DecodeRequest(CPacketHeader &header, char *p);
		int DecodeFieldValue(char *d, int l, int m);
		int DecodeFieldSet(char *d, int l);
		int DecodeResultSet(char *d, int l);

	public:
		CTask(CTableDefinition *tdef=NULL, CTaskRole r=TaskRoleServer, int final=0) :
			CTableReference(tdef),
			migratebuf(NULL),
			stage(final?DecodeStageDataError:DecodeStageIdle),
			role(r),
			dataTableDef(tdef),
			hotbackupTableDef(NULL),
			updateInfo(NULL),
			conditionInfo(NULL),
			fieldList(NULL),
			result(NULL),
			serialNr(0),
			key(NULL),
			rkey(NULL),
			resultWriter(NULL),
			resultWriterReseted(0),
			requestCode(0),
			requestType(0),
			requestFlags(0),
			replyCode(0),
			replyFlags(0),
			processFlags(PFLAG_ALLROWS)
			{
			}

		virtual ~CTask(void) {
			DELETE(updateInfo);
			DELETE(conditionInfo);
			DELETE(fieldList);
			DELETE(resultWriter);
			DELETE(result);
			FREE_IF(migratebuf);
		}
		// linked clone
		inline CTask(const CTask &orig) { CTask(); Copy(orig); }

		// these Copy()... only apply to empty CTask
		// linked clone
		int Copy(const CTask &orig);
		// linked clone with replace key
		int Copy(const CTask &orig, const CValue *newkey);
		// replace row
		int Copy(const CRowValue &);
		// internal API
		int Copy(NCRequest &rq, const CValue *key);

		/* newman: pool */
		inline void Clean()
		{
		    CTableReference::SetTableDefinition(NULL);
		    //stage = DecodeStageIdle;
		    //role = TaskRoleServer;
		    //dataTableDef = NULL;
		    //hotbackupTableDef = NULL;
		    if(updateInfo)
			updateInfo->Clean();
		    if(conditionInfo)
			conditionInfo->Clean();
		    if(fieldList)
			fieldList->Clean();
		    if(result)
			result->Clean();
		    versionInfo.Clean();
		    requestInfo.Clean();
		    resultInfo.Clean();

		    //serialNr = 0;
		    key = NULL;
		    rkey = NULL;
		    if(resultWriter)
		    {
			resultWriter->Clean();
			resultWriterReseted = 0;
		    }
		    requestCode = 0;
		    requestType = 0;
		    requestFlags = 0;
		    replyCode = 0;
		    replyFlags = 0;
		    processFlags = PFLAG_ALLROWS;
		}

		//////////// some API access request property
		/* newman: pool */
		inline void SetDataTable(CTableDefinition *t) { CTableReference::SetTableDefinition(t); dataTableDef = t;}
		inline void SetHotbackupTable(CTableDefinition *t) { hotbackupTableDef = t; }
#if 0
		CTableDefinition *TableDefinition(void) const { return tableDef; }
		int KeyFormat(void) const { return tableDef->KeyFormat(); }
		int KeyFields(void) const { return tableDef->KeyFields(); }
		int KeyAutoIncrement(void) const { return tableDef->KeyAutoIncrement(); }
		int AutoIncrementFieldId(void) const { return tableDef->AutoIncrementFieldId(); }
		int FieldType(int n) const { return tableDef->FieldType(n); }
		int FieldOffset(int n) const { return tableDef->FieldType(n); }
		int FieldId(const char *n) const { return tableDef->FieldId(n); }
		const char* FieldName(int id) const { return tableDef->FieldName(id); }
#endif

		// This code has to value (not very usefull):
		// DRequest::ResultInfo --> result/error code/key only
		// DRequest::ResultSet  --> ResultCode() >=0, with ResultSet
		// please use ResultCode() for detail error code
		int ReplyCode(void) const { return replyCode; }

		// Retrieve request key
		int HasRequestKey(void) const { return key != NULL; }
		const CValue *RequestKey(void) const { return key; }
		unsigned int IntKey(void)const{return (unsigned int)(RequestKey()->u64);}
		void UpdateKey(CRowValue *r) { (*r)[0] = *RequestKey(); }

		const CFieldValue *RequestCondition(void) const { return conditionInfo;}
		const CFieldValue *RequestOperation(void) const { return updateInfo;}

		//for migrate
		void SetRequestOperation(CFieldValue * ui){updateInfo = ui;}
		const CFieldSet *RequestFields(void) const { return fieldList;}
		const uint64_t RequestSerial(void) const { return serialNr;}

		// result key
		const CValue *ResultKey(void) const { return rkey; }
		// only if insert w/o key
		void SetResultKey(const CValue &v) {
			resultInfo.SetKey(v);
			rkey = &v;
		}

		static int MaxHeaderSize(void) { return sizeof(CPacketHeader); }
		static int MinHeaderSize(void) { return sizeof(CPacketHeader); }
		static int CheckPacketSize(const char *buf, int len);

		// Decode data from fd
		void DecodeStream(CSimpleReceiver &receiver);
		CDecodeResult Decode(CSimpleReceiver &receiver) {
			DecodeStream(receiver);
			return GetDecodeResult();
		}

		// Decode data from packet
		//     type 0: clone packet
		//     type 1: eat(keep&free) packet
		//     type 2: use external packet
		void DecodePacket(char *packetIn, int packetLen, int type);
		CDecodeResult Decode(char *packetIn, int packetLen, int type) {
			DecodePacket(packetIn, packetLen, type);
			return GetDecodeResult();
		}

		CDecodeResult Decode(const char *packetIn, int packetLen) {
			DecodePacket((char *)packetIn, packetLen, 0);
			return GetDecodeResult();
		};

		inline void BeginStage() { stage = DecodeStageIdle; }

		// change role from TaskRoleServer to TaskRoleHelperReply
		// cleanup decode state, prepare reply from helper
		inline void PrepareDecodeReply(void) {
			role = TaskRoleHelperReply;
			stage = DecodeStageIdle;
		}

		inline void SetRoleAsServer() { role = TaskRoleServer; }

		// set error code before CPacket::EncodeResult();
		// err is positive errno
		void SetError(int err, const char *from, const char *msg) {
			resultInfo.SetError(err, from, msg);
		}
		void SetErrorDup(int err, const char *from, const char *msg) {
			resultInfo.SetErrorDup(err, from, msg);
		}
		// retrieve previous result code
		// >= 0 success
		// < 0 err, negative value of SetError()
		int ResultCode(void) { return resultInfo.ResultCode(); }
		int AllowRemoteTable(void) const { return processFlags & PFLAG_ALLOWREMOTETABLE; }
		void MarkAllowRemoteTable(void) { processFlags |= PFLAG_ALLOWREMOTETABLE; }
		void MarkHasRemoteTable(void) { processFlags |= PFLAG_REMOTETABLE; }
		CTableDefinition *RemoteTableDefinition(void) {
			if(processFlags & PFLAG_REMOTETABLE) return TableDefinition();
			return NULL;
		}

		//////////// some API for request processing
		// Client Request Code
		int Role(void) const { return role; }
		int RequestCode(void) const { return requestCode; }
		int RequestType(void) const { return requestType; }
		void SetRequestType(CTaskType type) { requestType = type; }
		int FlagKeepAlive(void) const { return requestFlags & DRequest::Flag::KeepAlive; }
		int FlagTableDefinition(void) const { return requestFlags & DRequest::Flag::NeedTableDefinition; }
		int FlagNoCache(void) const { return requestFlags & DRequest::Flag::NoCache; }
		int FlagNoResult(void) const { return requestFlags & DRequest::Flag::NoResult; }
		int FlagNoNextServer(void) const { return requestFlags & DRequest::Flag::NoNextServer; }
		int FlagMultiKeyVal(void) const { return requestFlags & DRequest::Flag::MultiKeyValue; }
		int FlagMultiKeyResult(void) const { return replyFlags & DRequest::Flag::MultiKeyValue; }
		int FlagAdminTable(void) const { return requestFlags & DRequest::Flag::AdminTable; }
		int FlagPassThru(void) const { return processFlags & PFLAG_PASSTHRU; }
		int FlagFetchData(void) const { return processFlags & PFLAG_FETCHDATA; }
		int FlagIsHit(void) const { return processFlags & PFLAG_ISHIT; }
		int FlagFieldSetWithKey(void) const { return processFlags & PFLAG_FIELDSETWITHKEY; }
		int FlagBlackHole(void) const { return processFlags & PFLAG_BLACKHOLED; }
		void MarkAsPassThru(void) { processFlags |= PFLAG_PASSTHRU; }
		void MarkAsFetchData(void) { processFlags |= PFLAG_FETCHDATA; }
		void MarkAsHit(void) { processFlags |= PFLAG_ISHIT; }
		void MarkFieldSetWithKey(void) { processFlags |= PFLAG_FIELDSETWITHKEY; }
		void MarkAsBlackHole(void) { processFlags |= PFLAG_BLACKHOLED; }
		void SetResultHitFlag(CHITFLAG hitFlag){ resultInfo.SetHitFlag((uint32_t)hitFlag);}
		
		// this is count only request
		int CountOnly(void) const { return fieldList==NULL; }
		// this is non-contional request
		void ClearAllRows(void) { processFlags &= ~PFLAG_ALLROWS; }
		int AllRows(void) const { return processFlags & PFLAG_ALLROWS; }
		// apply insertValues, updateOperations to row
		int UpdateRow(CRowValue &row) {
			return updateInfo==NULL ? 0 : updateInfo->Update(row);
		}
		// checking the condition
		int CompareRow(const CRowValue &row, int iCmpFirstNRows=256) const {
			return AllRows() ? 1 : conditionInfo->Compare(row, iCmpFirstNRows);
		}

		// prepare an ResultSet, for afterward operation
		int PrepareResult(int start, int count);
		inline int PrepareResult(void) {
			return PrepareResult(requestInfo.LimitStart(), requestInfo.LimitCount());
		}
		inline int PrepareResultNoLimit(void) {
			return PrepareResult(0, 0);
		}

		inline void DetachResultInResultWriter() { if(resultWriter) resultWriter->DetachResult(); }

		// new a countonly ResultSet with 'nr' rows
		// No extra AppendRow() allowed
		int SetTotalRows(unsigned int nr, int Force=0) {
			if(!Force){
				if(resultWriter || PrepareResult()==0 )
					return resultWriter->SetRows(nr);
			}
			else{
				resultWriter->SetTotalRows(nr);
			}
			return 0;
		}
		void AddTotalRows(int n) { resultWriter->AddTotalRows(n); }
		int InRange(unsigned int nr, unsigned int begin=0) const { return resultWriter->InRange(nr, begin); }
		int ResultFull(void) const { return resultWriter && resultWriter->IsFull(); }
		// AppendRow, from row 'r'
		int AppendRow(const CRowValue &r) { return resultWriter->AppendRow(r); }
		int AppendRow(const CRowValue *r) { return r ? resultWriter->AppendRow(*r) : 0; }

		// Append Row from ResultSet with condition operation
		int AppendResult(CResultSet *rs);
		// Append All Row from ResultSet
		int PassAllResult(CResultSet *rs);
		// Merge all row from sub-task
		int MergeResult(const CTask& task);
		// Get Encoded Result Packet
		CResultPacket *GetResultPacket(void) { return (CResultPacket *)resultWriter; }
		// Process Internal Results
		int ProcessInternalResult(uint32_t ts=0);

};

extern int PacketBodyLen(CPacketHeader & header); 

#endif


