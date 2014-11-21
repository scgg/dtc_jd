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
    //����ʵ�������ʵ����δ���죬����һ���µ�ʵ������
    static CServerControl * GetInstance(CPollThread *o);
    //���Ƿ��أ����ʵ����δ���죬�򷵻ؿ�
    static CServerControl * GetInstance();
    virtual ~CServerControl(void);
    void BindDispatcher(CTaskDispatcher<CTaskRequest> *p) { m_output.BindDispatcher(p); }
    bool IsReadOnly();
private:
    CRequestOutput<CTaskRequest> m_output;
    //server�Ƿ�Ϊֻ��״̬
    atomic8_t m_readOnly;
    //Readonly��ͳ�ƶ���
    CStatItemU32 m_statReadonly;
private:
    virtual void TaskNotify(CTaskRequest *);
    //����serveradmin ����
    void dealServerAdmin(CTaskRequest *cur);
    void QueryMemInfo(CTaskRequest *cur);

};


#endif
