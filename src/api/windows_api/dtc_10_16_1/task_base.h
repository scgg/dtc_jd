#ifndef __CH_TASK_H__
#define __CH_TASK_H__

#include "tabledef.h"
#include "section.h"
#include "cache_error.h"
#include "buffer.h"
#include "receiver.h"
#include "field.h"
#include "result.h"



#define DELETE(pointer) \
	do \
	{ \
	if(pointer) { \
	delete pointer; \
	pointer = 0; \
	} \
	} while(0)


enum CTaskRole {
	TaskRoleServer=0, /* server, for incoming thread */
	TaskRoleClient,   /* client, reply from server */
	TaskRoleHelperReply, /* helper, reply from server */
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

enum CDecodeResult {
	DecodeFatalError,	// no response needed
	DecodeDataError,	// response with error code
	DecodeIdle,		// no data received
	DecodeWaitData,		// partial decoding
	DecodeDone		// full packet
};
enum CTaskType {
	TaskTypeAdmin = 0,
	TaskTypeRead,    /* Read Only operation */
	TaskTypeWrite,   /* Modify data */
	TaskTypeCommit,  /* commit dirty data */
};


class CTask: public CTableReference {
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
	CTask(CTableDefinition *tdef=NULL, CTaskRole r=TaskRoleServer, int final=0) :
		CTableReference(tdef),
		migratebuf(NULL),
		stage(final?DecodeStageDataError:DecodeStageIdle),
		role(r),
		dataTableDef(tdef),
		//hotbackupTableDef(NULL),
		updateInfo(NULL),
		conditionInfo(NULL),
		fieldList(NULL),
		result(NULL),
		serialNr(0),
		key(NULL),
		rkey(NULL),
		//resultWriter(NULL),
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
		//DELETE(resultWriter);
		DELETE(result);
		FREE_IF(migratebuf);
	}


protected: // packet info, read-only
//	CFieldValue *updateInfo;
//	CFieldValue *conditionInfo;
//	CFieldSet *fieldList;

public:
	char * migratebuf;
protected: // decoder informations
	CDecodeStage stage;
	CTaskRole role;
	BufferPool packetbuf;

	CTableDefinition *dataTableDef;

	// linked clone
	inline CTask(const CTask &orig) { CTask(); Copy(orig); }

	// these Copy()... only apply to empty CTask
	// linked clone
	int Copy(const CTask &orig);



	//////////// some API for request processing
	// Client Request Code




public:
	int Role(void) const { return role; }
	int ResultCode(void) { return resultInfo.ResultCode(); }




//Decode data
public:
	void DecodeStream(CSimpleReceiver &receiver);
	CDecodeResult Decode(CSimpleReceiver &receiver) {
		DecodeStream(receiver);
		return GetDecodeResult();
	}
	int AllowRemoteTable(void) const { return processFlags & PFLAG_ALLOWREMOTETABLE; }
	void MarkHasRemoteTable(void) { processFlags |= PFLAG_REMOTETABLE; }
	int DecodeFieldValue(char *d, int l, int m);
	int DecodeFieldSet(char *d, int l);
	int DecodeResultSet(char *d, int l);
	void ClearAllRows(void) { processFlags &= ~PFLAG_ALLROWS; }
	void MarkAllowRemoteTable(void) { processFlags |= PFLAG_ALLOWREMOTETABLE; }
	CTableDefinition *RemoteTableDefinition(void) {
		if(processFlags & PFLAG_REMOTETABLE) return TableDefinition();
		return NULL;
	}

//Decode data
protected:
	int DecodeHeader(const CPacketHeader &in, CPacketHeader *out = NULL);
	void DecodeRequest(CPacketHeader &header, char *p);
	int resultWriterReseted;
	uint8_t requestCode; /* derived from packet */
	uint8_t requestType; /* derived from packet */
	uint8_t requestFlags; /* derived from packet */
	uint8_t replyCode; /* processing */
	uint8_t replyFlags; /* derived from packet */
	uint64_t serialNr; /* derived from packet */
	int ValidateSection(CPacketHeader &header);
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

public:
		static const CDecodeResult stage2result[];
		CDecodeResult GetDecodeResult(void) {return stage2result[stage];};
		static const uint32_t validcmds[];
		static const uint16_t cmd2type[];
		static const uint16_t validsections[][2];
		static const uint8_t validktype[DField::TotalType][DField::TotalType];
		static const uint8_t validxtype[DField::TotalOperation][DField::TotalType][DField::TotalType];
		static const uint8_t validcomps[DField::TotalType][DField::TotalComparison];

public: // packet info, read-write
		//CPacketHeader headerInfo;
		CVersionInfo versionInfo;
		CRequestInfo requestInfo;
		CResultInfo resultInfo;

		const CValue *key; /* derived from packet */
		const CValue *rkey; /* processing */

		// result key
		const CValue *ResultKey(void) const { return rkey; }

protected: // packet info, read-only
		CFieldValue *updateInfo;
		CFieldValue *conditionInfo;
		CFieldSet *fieldList;
	public:
		CResultSet *result;
};










#endif