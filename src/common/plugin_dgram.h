#ifndef __PLUGIN__DGRAM_H__
#define __PLUGIN__DGRAM_H__

#include <sys/socket.h>

#include "poller.h"
#include "timerlist.h"
#include "plugin_decoder.h"
#include "plugin_unit.h"

class CPluginDatagram;

class CPluginDgram : public CPollerObject
{
public:
	CPluginDgram (CPluginDecoderUnit *, int fd);
	virtual ~CPluginDgram ();

	virtual int Attach (void);
    inline int send_response (CPluginDatagram* plugin_request)
    {
        int ret = 0;
        _owner->RecordRequestTime(plugin_request->_response_timer.live());

        if (!plugin_request->recv_only())
        {
            ret = _plugin_sender.sendto (plugin_request);
        }

        DELETE (plugin_request);
        return ret;
    }

private:
	virtual void InputNotify (void);

protected:	
	int RecvRequest (void);
    int init_request (void);

private:
    int                 mtu;
    int                 _addr_len;
	CPluginDecoderUnit* _owner;
    worker_notify_t*    _worker_notifier;
    CPluginReceiver     _plugin_receiver;
    CPluginSender       _plugin_sender;
    uint32_t            _local_ip;

private:
	int InitSocketInfo(void);
};

#endif

