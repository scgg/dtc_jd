/*
 * =====================================================================================
 *
 *       Filename:  front_worker.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2010 05:45:54 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef AG_WORKER_H__
#define AG_WORKER_H__

#include "poller.h"
#include "timerlist.h"
#include "hitinfo.h"
#include "module_acceptor.h"
#include <map>
#include <string>


class CModule;
class CPollThread;
class CAGSender;
class CAGReceiver;

class CFrontWorker: public CPollerObject, public CTimerObject
{
public:
	CFrontWorker(){};
    CFrontWorker(uint64_t id, int fd, CPollThread *ownerThread, CModule *ownerModule,CStatManager *manager);
    ~CFrontWorker();

    int SendResponse(char *buf, uint32_t len, CHitInfo &hitInfo, uint64_t pkgSeq);
    bool isValidFrontworker()
    {
    	return netfd;
    }
    uint32_t Id() { return m_id; }

private:
    uint32_t     m_id;
    uint32_t     m_moduleId;
    CModule     *m_ownerModule;
    CAGSender   *m_sender;
    CAGReceiver *m_receiver;
    CPollThread *m_ownerThread;
    CStatManager *m_statManager;
    CStatItem *m_statItem;
    //璇锋眰鑰楁椂鍙橀噺鍗曚綅ms linjinming 2014-05-23
    std::map<uint64_t, uint64_t> m_pkg2Elapse;//worker
    CTimerList 		   *timerList;//woker
public:
    virtual void InputNotify();
    virtual void OutputNotify();
    virtual void TimerNotify();
    //父类的HangupNotify方法
    virtual void HangupNotify (void);

    int ExtractPackedKey(char *buf, uint32_t len,
            char **packedKey, uint32_t *packedKeyLen, std::string &accessToken,bool &mark , uint64_t &agentId);
    int EncodeAgentIdToNewRequest(uint64_t id, char *buf, uint32_t len,
            char **newPackedKey, uint32_t *newPacketLen, uint64_t &pkgSeq);
    void ReplyWithError(const char *errMsg, char *packet);
    void EXIT();
    int Send();
    int Recv();
};

#endif
