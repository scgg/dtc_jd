#ifndef ADMIN_WORKER_H__
#define ADMIN_WORKER_H__

#include <vector>

#include "poller.h"
#include "timerlist.h"
#include "admin.h"
#include "sender.h"
#include "poll_thread_group.h"
#include "CFdDispacher.h"

class CPollThread;
class CBussinessModuleMgr;

class CAdminWorker: public CPollerObject
{
public:
    CAdminWorker(int fd, CPollThread *ownerThread, CBussinessModuleMgr *mgr,CPollThreadGroup *threadgroup ,CConfig* gConfigObj,CFdDispacher *gFdDispacher);

    virtual void InputNotify();
    virtual void OutputNotify();
private:
    void ProcessRequest();
    void ProcessAddModule(uint32_t module, const char *listenAddr, const char *accessToken);
    void ProcessRemoveModule(uint32_t module);
    void ProcessAddCacheServer(uint32_t module, uint32_t id,
    		const char *name, const char *addr, int virtualNode,const char *hotbak_addr);
    void ProcessRemoveCacheServer(uint32_t module,
        const char *name, int virtualNode);

    void ProcessChangeCacheServerAddress(uint32_t module,
    		const char *name, const char *addr,const char *hotbak_addr);
    void ProcessReloadConfig(const char *config);
    void ReplyWithMessage(uint8_t code, const char *msg);
    void ReplyOk();
    void ProcessPing(CPollThreadGroup *threadgroup,CFdDispacher *gFdDispacher);
    void ProcessSwitchCacheServer(uint32_t module,
    		std::string name, uint32_t mode);

    std::vector<char> m_request;
    ProtocolHeader m_header;
    uint32_t m_offset;
    CAGSender *m_sender;
    CBussinessModuleMgr *m_moduleMgr;
    CPollThread *m_ownerThread;
    CPollThreadGroup *m_threadgroup;
    CConfig* m_gConfigObj;
    CFdDispacher* m_gFdDispacher;
};

#endif
