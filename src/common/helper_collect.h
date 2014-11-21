#ifndef __HELPER_COLLECT_H_
#define __HELPER_COLLECT_H__

#include "dbconfig.h"
#include "poll_thread.h"
#include "request_base.h"
#include <StatTTC.h>

class CHelperGroup;
class CTimerList;
class CKeyGuard;


enum 
{
	MASTER_READ_GROUP_COLUMN = 0,
	MASTER_WRITE_GROUP_COLUMN = 1,
	MASTER_COMMIT_GROUP_COLUMN = 2,
	SLAVE_READ_GROUP_COLUMN = 3,
};
class CGroupCollect : public CTaskDispatcher<CTaskRequest>
{
public:
	CGroupCollect ();
	~CGroupCollect ();

	int LoadConfig (const struct CDbConfig *cfg, int ks);
	bool IsCommitFull (CTaskRequest *task);
	bool Dispatch (CTaskRequest *t);
	int Attach(CPollThread *thread);
	void SetTimerHandler(CTimerList *recv, CTimerList *conn, CTimerList *retry);
	int DisableCommitGroup(void);

	int HasDummyMachine(void) const { return hasDummyMachine; }
private:
	virtual void TaskNotify(CTaskRequest *);
	CHelperGroup *SelectGroup (CTaskRequest *t);

	void StatHelperGroupQueueCount(CHelperGroup **group, unsigned group_count);
	void StatHelperGroupCurMaxQueueCount(int iRequestType);
	int GetQueueCurMaxCount(int iColumn);
private:
	const struct CDbConfig *dbConfig; 

	int hasDummyMachine;
	
	CHelperGroup **groups;
#define GMAP_NONE  -1
#define GMAP_DUMMY -2
#define GROUP_DUMMY ((CHelperGroup *)-2)
	short *groupMap;
	CReplyDispatcher<CTaskRequest> *guardReply;
	CLinkQueue<CTaskRequest *>::allocator taskQueueAllocator;

public:
	CKeyGuard *guard;

private:
	CStatItemU32 statQueueCurCount;/*所有组当前总的队列大小*/
	CStatItemU32 statQueueMaxCount;/*所有组配置总的队列大小*/
	CStatItemU32 statReadQueueCurMaxCount;/*所有机器所有主读组当前最大的队列大小*/
	CStatItemU32 statWriteQueueMaxCount;/*所有机器所有写组当前最大的队列大小*/
	CStatItemU32 statCommitQueueCurMaxCount;/*所有机器所有提交组当前最大的队列大小*/
	CStatItemU32 statSlaveReadQueueMaxCount;/*所有机器所有备读组当前最大的队列大小*/
};

#endif
