/*
 * =====================================================================================
 *
 *       Filename:  plugin_decoder.h
 *
 *    Description:  实现 plugin 网络状态机管理
 *
 *        Version:  1.0
 *        Created:  11/24/2009 10:33:34 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef __TTC_PLUGIN_DECODER_H__
#define __TTC_PLUGIN_DECODER_H__

#include "noncopyable.h"
#include "plugin_global.h"
#include "plugin_request.h"

typedef enum
{
    NET_IDLE,
    NET_FATAL_ERROR,
    NET_DISCONNECT,
    NET_RECVING,
    NET_SENDING,
    NET_SEND_DONE,
    NET_RECV_DONE,

} net_state_t;

class CPluginReceiver: private noncopyable
{
public:
    CPluginReceiver (int fd, dll_func_t* dll) : _fd (fd), _stage(NET_IDLE), _dll (dll), _all_len_changed (0)
    {

    }
    ~CPluginReceiver (void)
    {

    }

    int recv (CPluginStream* request);
    int recvfrom (CPluginDatagram* request, int mtu);
    inline int proc_remain_packet (CPluginStream* request)
    {
        request->recalc_multipacket ();
        return parse_protocol (request);
    }

    inline void set_stage (net_state_t stage)
    {
        _stage = stage;
    }

public:

protected:
protected:

private:
    int parse_protocol (CPluginStream* request);

private:
    int         _fd;
    net_state_t _stage;
    dll_func_t* _dll; 
    int         _all_len_changed;
};

class CPluginSender : private noncopyable
{
public:
    CPluginSender (int fd, dll_func_t* dll) : _fd (fd), _stage(NET_IDLE), _dll (dll)
    {

    }

    ~CPluginSender (void)
    {

    }

    int send (CPluginStream* request);
    int sendto (CPluginDatagram* request);

    inline void set_stage (net_state_t stage)
    {
        _stage = stage;
    }

public:

protected:
protected:

private:
private:
    int         _fd;
    net_state_t _stage;
    dll_func_t* _dll; 
};


#endif
