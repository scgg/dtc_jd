#include <errno.h>
#include <unistd.h>

#include "client_unit.h"
#include "client_sync.h"
#include "client_async.h"
#include "client_dgram.h"
#include "poll_thread.h"
#include "task_request.h"
#include "log.h"

CTTCDecoderUnit::CTTCDecoderUnit(CPollThread *o, CTableDefinition **tdef, int it) :
	CDecoderUnit(o, it),
	tableDef(tdef),
	output(o)
{

	statRequestTime[0] = statmgr.GetSample(REQ_USEC_ALL);
	statRequestTime[1] = statmgr.GetSample(REQ_USEC_GET);
	statRequestTime[2] = statmgr.GetSample(REQ_USEC_INS);
	statRequestTime[3] = statmgr.GetSample(REQ_USEC_UPD);
	statRequestTime[4] = statmgr.GetSample(REQ_USEC_DEL);
	statRequestTime[5] = statmgr.GetSample(REQ_USEC_FLUSH);
	statRequestTime[6] = statmgr.GetSample(REQ_USEC_HIT);
	statRequestTime[7] = statmgr.GetSample(REQ_USEC_REPLACE);

	/* newman: pool */
	if(clientResourcePool.Init() < 0)
	    throw (int) -ENOMEM;
}

CTTCDecoderUnit::~CTTCDecoderUnit() {
}

void CTTCDecoderUnit::RecordRequestTime(int hit, int type, unsigned int usec)
{
	static const unsigned char cmd2type[] =
	{
	    /*Nop*/ 0,
	    /*ResultCode*/ 0,
	    /*ResultSet*/ 0,
	    /*HelperAdmin*/ 0,
	    /*Get*/ 1,
	    /*Purge*/ 5,
	    /*Insert*/ 2,
	    /*Update*/ 3,
	    /*Delete*/ 4,
		/*Other*/ 0,
	    /*Other*/ 0,
	    /*Other*/ 0,
	    /*Replace*/ 7,
	    /*Flush*/ 5,
	    /*Other*/ 0,
	    /*Other*/ 0,
	};
	statRequestTime[0].push(usec);
	unsigned int t = hit ? 6 : cmd2type[type];
	if(t) statRequestTime[t].push(usec);
}

void CTTCDecoderUnit::RecordRequestTime(CTaskRequest *req) {
	RecordRequestTime(req->FlagIsHit(), req->RequestCode(), req->responseTimer.live());
}

int CTTCDecoderUnit::ProcessStream(int newfd, int req, void *peer, int peerSize)
{
	if(req <= 1)
	{
		/* newman: pool */
        CClientSync* cli = NULL;
		try { cli = new CClientSync(this, newfd, peer, peerSize); }
        catch(int err) {DELETE(cli); return -1;}
 
		if (0 == cli)
		{
			log_error("create CClient object failed, errno[%d], msg[%m]", errno);
			return -1;
		}

		if (cli->Attach () == -1) 
		{
			log_error("Invoke CClient::Attach() failed");
			delete cli;
			return -1;
		}

		/* accept唤醒后立即recv */
		cli->InputNotify();
	} else {
		CClientAsync* cli = new CClientAsync(this, newfd, req, peer, peerSize);

		if (0 == cli)
		{
			log_error("create CClient object failed, errno[%d], msg[%m]", errno);
			return -1;
		}

		if (cli->Attach () == -1) 
		{
			log_error("Invoke CClient::Attach() failed");
			delete cli;
			return -1;
		}

		/* accept唤醒后立即recv */
		cli->InputNotify();
	}
	return 0;
}

int CTTCDecoderUnit::ProcessDgram(int newfd)
{
	CClientDgram* cli = new CClientDgram(this, newfd);

	if (0 == cli)
	{
		log_error("create CClient object failed, errno[%d], msg[%m]", errno);
		return -1;
	}

	if (cli->Attach () == -1) 
	{
		log_error("Invoke CClient::Attach() failed");
		delete cli;
		return -1;
	}
	return 0;
}

/* newman: pool */
int CTTCDecoderUnit::RegistResource(CClientResourceSlot ** res, unsigned int & id, uint32_t & seq)
{
    if(clientResourcePool.Alloc(id, seq) < 0)
    {
        id = 0;
        *res = NULL;
        return -1;
    }

    *res = clientResourcePool.Slot(id);
    return 0;
}

void CTTCDecoderUnit::UnregistResource(unsigned int id, uint32_t seq)
{
    clientResourcePool.Free(id, seq);
}

void CTTCDecoderUnit::CleanResource(unsigned int id)
{
    clientResourcePool.Clean(id);
}
