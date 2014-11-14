#ifndef __H_MULTI_REQUEST_H__
#define __H_MULTI_REQUEST_H__

#include "request_base_all.h"

class CTaskMultiplexer;
class CTaskRequest;
union CValue;

class CMultiRequest
{
	private:
		CTaskMultiplexer *owner;
		CTaskRequest *wait;
		CValue *keyList;
		unsigned char *keyMask;
		int doneReq;
		int totalReq;
		int subReq;
		int firstPass;
		int keyFields;
		int internal;

	public:
		friend class CTaskMultiplexer;
		CMultiRequest(CTaskMultiplexer *o, CTaskRequest* task);
		~CMultiRequest(void);

		int DecodeKeyList(void);
		int SplitTask(void);
		int TotalCount(void) const { return totalReq; }
		int RemainCount(void) const { return totalReq - doneReq; }
		void SecondPass(int err);
		void CompleteTask(CTaskRequest *req, int index);

	public:
		CValue *GetKeyValue(int i);
		void SetKeyCompleted(int i);
		int IsKeyCompleted(int i);
		void CompleteWaiter(void);

};

class CMultiTaskReply : public CReplyDispatcher<CTaskRequest> {
    public:
	CMultiTaskReply(){}
	virtual void ReplyNotify(CTaskRequest *cur);
};

#endif
