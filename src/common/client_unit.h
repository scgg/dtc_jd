#ifndef __H_TTC_CLIENT_UNIT_H__
#define __H_TTC_CLIENT_UNIT_H__

#include "request_base_all.h"
#include "StatTTC.h"
#include "decoder_base.h"
#include "stopwatch.h"
#include "client_resource_pool.h"

class CTaskRequest;
class CTableDefinition;

class CTTCDecoderUnit: public CDecoderUnit
{
	public:
		CTTCDecoderUnit(CPollThread *, CTableDefinition **, int);
		virtual ~CTTCDecoderUnit();

		virtual int ProcessStream(int fd, int req, void *, int);
		virtual int ProcessDgram(int fd);

		void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { output.BindDispatcher(p); }
		void TaskDispatcher(CTaskRequest *task) { output.TaskNotify(task); }
		CTableDefinition *OwnerTable(void) const { return tableDef[0]; }
		CTableDefinition *AdminTable(void) const { return tableDef[1]; }

		void RecordRequestTime(int hit, int type, unsigned int usec);
		// CTaskRequest *p must nonnull
		void RecordRequestTime(CTaskRequest *p);
		void RecordRcvCnt(void);
		void RecordSndCnt(void);
		/* newman: pool */
		int RegistResource(CClientResourceSlot ** res, unsigned int & id, uint32_t & seq);
		void UnregistResource(unsigned int id, uint32_t seq);
		void CleanResource(unsigned int id);
	private:

		CTableDefinition **tableDef;
		CRequestOutput<CTaskRequest> output;
		/* newman: pool*/
		CClientResourcePool clientResourcePool;

		CStatSample statRequestTime[8];
};

#endif
