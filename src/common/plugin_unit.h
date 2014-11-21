#ifndef __H_TTC_PLUGIN_UNIT_H__
#define __H_TTC_PLUGIN_UNIT_H__

#include "StatTTC.h"
#include "decoder_base.h"
#include "plugin_request.h"
#include "poll_thread.h"

class CPluginDecoderUnit: public CDecoderUnit
{
public:
    CPluginDecoderUnit(CPollThread* owner, int idletimeout);
    virtual ~CPluginDecoderUnit();

    virtual int ProcessStream(int fd, int req, void *peer, int peerSize);
    virtual int ProcessDgram(int fd);

    inline void RecordRequestTime(unsigned int msec)
    {
        //statRequestTime[0].push (msec);
    }

    inline incoming_notify_t* get_incoming_notifier (void)
    {
        return &_incoming_notify;
    }

    inline int AttachIncomingNotifier (void)
    {
        return _incoming_notify.AttachPoller (owner);
    }

private:
    //CStatSample         statRequestTime[1];
    incoming_notify_t   _incoming_notify;
};

#endif
