#ifndef __PLUGIN_SYNC_H__
#define __PLUGIN_SYNC_H__

#include "poller.h"
#include "timerlist.h"
#include "plugin_unit.h"
#include "plugin_decoder.h"

class CPluginDecoderUnit;

typedef enum
{
    PLUGIN_IDLE,
    PLUGIN_RECV,        //wait for recv request, server side
    PLUGIN_SEND,        //wait for send response, server side
    PLUGIN_PROC         //IN processing
} plugin_state_t;

class CPluginSync : public CPollerObject, private CTimerObject
{
public:
	CPluginSync (CPluginDecoderUnit* plugin_decoder, int fd, void* peer, int peer_size);
	virtual ~CPluginSync ();

	int Attach (void);
	virtual void InputNotify (void);

    inline int send_response (void)
    {
        owner->RecordRequestTime(_plugin_request->_response_timer.live());
        if (Response() < 0)
        {
            delete this;
        }
        else
        {
	    DelayApplyEvents ();
        }

        return 0;
    }

    inline void set_stage (plugin_state_t stage)
    {
        _plugin_stage = stage;
    }

private:
	virtual void OutputNotify (void);

protected:	
	plugin_state_t  _plugin_stage;
	
	int RecvRequest (void);
	int Response (void);

private:
    int create_request (void);
    int proc_multi_request (void);

private:
    CPluginDecoderUnit* owner;
    CPluginStream*      _plugin_request;
    worker_notify_t*    _worker_notifier;
    void*               _addr;
    int                 _addr_len;
    CPluginReceiver     _plugin_receiver;
    CPluginSender       _plugin_sender;
};

#endif
