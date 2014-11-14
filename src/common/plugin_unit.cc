#include <errno.h>
#include <unistd.h>

#include "plugin_unit.h"
#include "plugin_sync.h"
#include "plugin_dgram.h"
#include "log.h"
#include "memcheck.h"

CPluginDecoderUnit::CPluginDecoderUnit(CPollThread *o, int it) :
	CDecoderUnit(o, it)
{
	//statRequestTime[0] = statmgr.GetSample(PLUGIN_REQ_USEC_ALL);
}

CPluginDecoderUnit::~CPluginDecoderUnit()
{

}

int CPluginDecoderUnit::ProcessStream(int newfd, int req, void *peer, int peerSize)
{
    CPluginSync* plugin_client = NULL;
    NEW (CPluginSync(this, newfd, peer, peerSize), plugin_client);

    if (0 == plugin_client)
    {
        log_error("create CPluginSync object failed, errno[%d], msg[%m]", errno);
        return -1;
    }

    if (plugin_client->Attach () == -1)
    {
        log_error("Invoke CPluginSync::Attach() failed");
        delete plugin_client;
        return -1;
    }

    /* accept唤醒后立即recv */
    plugin_client->InputNotify();

    return 0;
}

int CPluginDecoderUnit::ProcessDgram(int newfd)
{
	CPluginDgram* plugin_dgram = NULL;
    NEW (CPluginDgram(this, newfd), plugin_dgram);

	if (0 == plugin_dgram)
	{
		log_error("create CPluginDgram object failed, errno[%d], msg[%m]", errno);
		return -1;
	}

	if (plugin_dgram->Attach () == -1)
	{
		log_error("Invoke CPluginDgram::Attach() failed");
		delete plugin_dgram;
		return -1;
	}

	return 0;
}
