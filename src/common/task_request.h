#ifndef __H_TTC_REQUEST_REAL_H__
#define __H_TTC_REQUEST_REAL_H__

#include "request_base_all.h"
#include "task_base.h"
#include "stopwatch.h"

class CDecoderUnit;
class CMultiRequest;
class NCKeyValueList;
class NCRequest;
class CAgentMultiRequest;
class CClientAgent;

class CTaskOwnerInfo {
private:
	const struct sockaddr *clientaddr;
	void *volatile ownerInfo;
	int ownerIndex;

public:
	CTaskOwnerInfo(void) : clientaddr(NULL), ownerInfo(NULL), ownerIndex(0) {}
	virtual ~CTaskOwnerInfo(void) {}

	void SetOwnerInfo(void *info, int idx, const struct sockaddr *addr) {
		ownerInfo = info;
		ownerIndex = idx;
		clientaddr = addr;
	}

	inline void Clean(){ownerInfo = NULL;}

	void ClearOwnerInfo(void) { ownerInfo = NULL; }

	template<typename T>
		T *OwnerInfo(void) const { return (T *)ownerInfo; }

	const struct sockaddr *OwnerAddress(void) const { return clientaddr; }
	int OwnerIndex(void) const { return ownerIndex; }
};

class CTaskRequest: public CTask, public CTaskReplyList<CTaskRequest, 10>, public CTaskOwnerInfo
{
public:
	CTaskRequest(CTableDefinition *t = NULL):
		CTask(t, TaskRoleServer),
		blacklist_size(0),
		timestamp(0),
		barHash(0),
		packedKey(NULL),
		expire_time(0),
		multiKey(NULL),
		keyList(NULL),
		batchKey(NULL),
		agentMultiReq(NULL),
                ownerClient(NULL),
                recvBuff(NULL),
                recvLen(0),
                recvPacketCnt(0),
		resourceId(0),
		resourceOwner(NULL),
		resourceSeq(0)
	{
	};

	virtual ~CTaskRequest();

	inline CTaskRequest(const CTaskRequest &rq) { CTaskRequest(); Copy(rq); }

	int Copy(const CTaskRequest &rq);
	int Copy(const CTaskRequest &rq, const CValue *newkey);
	int Copy(const CRowValue &);
	int Copy(NCRequest &, const CValue *);

public:
	void Clean();
	// msecond: absolute ms time
	uint64_t DefaultExpireTime(void) { return 5000 /*default:5000ms*/; }
	const uint64_t ExpireTime(void) const { return expire_time; }

	int IsExpired(uint64_t now) const {
		// client gone, always expired
		if(OwnerInfo<void>() == NULL)
			return 1;
		// flush cmd never time out
		if(requestType == TaskTypeCommit)
			return 0;
		return expire_time <= now;
	}
	uint32_t Timestamp(void) const { return timestamp; }
	void RenewTimestamp(void) { timestamp = time(NULL); }

	const char *PackedKey(void) const { return packedKey; }
	unsigned long BarrierKey (void) const { return barHash; }
	CValue *MultiKeyArray(void) { return multiKey; }

private:
	int BuildPackedKey(void);
	void CalculateBarrierKey(void);
public:
	int PrepareProcess(void);
	int UpdatePackedKey(uint64_t);
	void PushBlackListSize(const unsigned size) {blacklist_size = size;}
	unsigned PopBlackListSize(void) {register unsigned ret=blacklist_size; blacklist_size =0; return ret;}

	void DumpUpdateInfo(const char *prefix) const;
	void UpdateKey(CRowValue &r) {
		if(multiKey)
			r.CopyValue(multiKey, 0, KeyFields());
		else
			r[0] = *RequestKey();
	}
	void UpdateKey(CRowValue *r) {
		if(r) UpdateKey(*r);
	}
	int UpdateRow(CRowValue &row) {
		row.UpdateTimestamp(timestamp,
				requestCode!=DRequest::Update ||
				( updateInfo && updateInfo->HasTypeCommit()));
		return CTask::UpdateRow(row);
	}

    void SetRemoteAddr(const char *addr)
    {
        strncpy(remoteAddr, addr, sizeof(remoteAddr));
        remoteAddr[sizeof(remoteAddr) - 1] = 0;
    }

    const char *RemoteAddr() { return remoteAddr; }

private:
	/* following filed should be clean:
	 * blacklist_size
	 * timestamp
	 * barHash
	 * expire_time
	 *
	 * */
	/* 加入黑名单的大小 */
	unsigned blacklist_size;
	uint32_t timestamp;

	unsigned long barHash;
	char *packedKey;
	char packedKeyBuf[8];
	uint64_t expire_time; /* ms */ /* derived from packet */
	CValue *multiKey;
    char remoteAddr[32];

private:
	int BuildMultiKeyValues(void);
	int BuildSingleStringKey(void);
	int BuildSingleIntKey(void);
	int BuildMultiIntKey(void);

	void FreePackedKey(void) {
		if(packedKey  && packedKey != packedKeyBuf)
			FREE_CLEAR(packedKey);
		FREE_IF(multiKey);
	}
	/* for batch request*/
private:
	const NCKeyValueList *keyList;
	/* need clean when task begin in use(deleted when batch request finished) */
	CMultiRequest *batchKey;

        /* for agent request */
private:
        CAgentMultiRequest * agentMultiReq;
        CClientAgent * ownerClient;
	char * recvBuff;
        int recvLen;
        int recvPacketCnt;
public:
	unsigned int KeyValCount() const { return versionInfo.GetTag(11)?versionInfo.GetTag(11)->s64:1; }
	const CArray* KeyTypeList() const { return (CArray*)&(versionInfo.GetTag(12)->bin); }
	const CArray* KeyNameList() const { return (CArray*)&(versionInfo.GetTag(13)->bin); }
	const CArray* KeyValList() const { return (CArray*)&(requestInfo.Key()->bin); }
	const NCKeyValueList * InternalKeyValList() const { return keyList; }

	int IsBatchRequest(void) const { return batchKey != NULL; }
	int GetBatchSize(void) const;
	void SetBatchKeyList(CMultiRequest *bk) { batchKey = bk; }
	CMultiRequest *GetBatchKeyList(void) { return batchKey; }
	int SetBatchCursor(int i);
	void DoneBatchCursor(int i);

public:
        /* for agent request */
        void SetOwnerClient(CClientAgent * client);
        CClientAgent * OwnerClient();
        void ClearOwnerClient();

        int DecodeAgentRequest();
	inline void SaveRecvedResult(char * buff, int len, int pktcnt)
	{
		recvBuff = buff; recvLen = len; recvPacketCnt = pktcnt;
	}
        bool IsAgentRequestCompleted();
        void DoneOneAgentSubRequest();

	void LinkToOwnerClient(CListObject<CAgentMultiRequest> & head);

        int AgentSubTaskCount();
        void CopyReplyForAgentSubTask();
        CTaskRequest * CurrAgentSubTask(int index);

	bool IsCurrSubTaskProcessed(int index);
private:
	void PassRecvedResultToAgentMutiReq();

public:	// timing
	stopwatch_usec_t responseTimer;
	void ResponseTimerStart(void) { responseTimer.start(); }
	unsigned int resourceId;
	CDecoderUnit * resourceOwner;
	uint32_t resourceSeq;
};

#endif
