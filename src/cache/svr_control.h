#ifndef __REQUEST_CONTROL_H
#define __REQUEST_CONTROL_H

#include <task_request.h>
#include <StatTTC.h>

class CServerControl : public CTaskDispatcher<CTaskRequest>
{
protected:
    static CServerControl * serverControl;
    CServerControl(CPollThread *o);
    
public:
    //返回实例，如果实例尚未构造，则构造一个新的实例返回
    static CServerControl * GetInstance(CPollThread *o);
    //仅是返回，如果实例尚未构造，则返回空
    static CServerControl * GetInstance();
    virtual ~CServerControl(void);
    void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { m_output.BindDispatcher(p); }
    bool IsReadOnly();
private:
    CRequestOutput<CTaskRequest> m_output;
    //server是否为只读状态
    atomic8_t m_readOnly;
    //Readonly的统计对象
    CStatItemU32 m_statReadonly;
private:
    virtual void TaskNotify(CTaskRequest *);
    //处理serveradmin 命令
    void dealServerAdmin(CTaskRequest *cur);
    void QueryMemInfo(CTaskRequest *cur);

};


#endif
