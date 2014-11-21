
#include "list.h"
#include "dbconfig.h"
#include "helper_group.h"
#include "helper_collect.h"
#include "request_base.h"
#include "task_request.h"
#include "log.h"
#include "key_guard.h"
#include "StatTTC.h"


class CGuardNotify : public CReplyDispatcher<CTaskRequest> {
public:
	CGuardNotify(CGroupCollect *o) : owner(o) {}
	~CGuardNotify(){}
	virtual void ReplyNotify(CTaskRequest *);
private:
	CGroupCollect *owner;
};

void CGuardNotify::ReplyNotify(CTaskRequest *task) {
	if(task->ResultCode() >= 0)
		owner->guard->AddKey(task->BarrierKey(), task->PackedKey());
	task->ReplyNotify();
}

CGroupCollect::CGroupCollect () :
	CTaskDispatcher<CTaskRequest>(NULL),
	dbConfig(NULL),
	hasDummyMachine(0),
	groups(NULL),
	groupMap(NULL),
	guardReply(NULL),
	guard(NULL)
{
	/*总队列的统计，暂时还有意义，暂时保留*/
	statQueueCurCount = statmgr.GetItemU32(CUR_QUEUE_COUNT);
	statQueueMaxCount = statmgr.GetItemU32(MAX_QUEUE_COUNT);
	
	/*新增的四个组中最大的队列长度统计项，用来进行告警监控*/
	statReadQueueCurMaxCount = statmgr.GetItemU32(HELPER_READ_GROUR_CUR_QUEUE_MAX_SIZE);
	statWriteQueueMaxCount = statmgr.GetItemU32(HELPER_WRITE_GROUR_CUR_QUEUE_MAX_SIZE);
	statCommitQueueCurMaxCount = statmgr.GetItemU32(HELPER_COMMIT_GROUR_CUR_QUEUE_MAX_SIZE);
	statSlaveReadQueueMaxCount = statmgr.GetItemU32(HELPER_SLAVE_READ_GROUR_CUR_QUEUE_MAX_SIZE);
}

CGroupCollect::~CGroupCollect ()
{
	if (groups)
	{
		for (int i = 0; i < dbConfig->machineCnt * GROUPS_PER_MACHINE; i++)
			DELETE(groups[i]);

		FREE_CLEAR(groups);
	}

	FREE_CLEAR(groupMap);
	DELETE(guard);
	DELETE(guardReply);
}

CHelperGroup *CGroupCollect::SelectGroup(CTaskRequest *task)
{

	const CValue *key = task->RequestKey();
	uint64_t uk;
	/* key-hash disable */
	if(dbConfig->keyHashConfig.keyHashEnable == 0 || key == NULL){
		if(NULL == key)
			uk =0;
		else if(key->s64 < 0)
			uk = 0-key->s64;
		else
			uk = key->s64;
	}
	else{
		switch(task->FieldType(0)){
			case DField::Signed:
			case DField::Unsigned:
				uk = dbConfig->keyHashConfig.keyHashFunction((const char*)&(key->u64),
						sizeof(key->u64),
						dbConfig->keyHashConfig.keyHashLeftBegin,
						dbConfig->keyHashConfig.keyHashRightBegin);
				break;
			case DField::String:
			case DField::Binary:
				uk = dbConfig->keyHashConfig.keyHashFunction(key->bin.ptr,
						key->bin.len,
						dbConfig->keyHashConfig.keyHashLeftBegin,
						dbConfig->keyHashConfig.keyHashRightBegin);
				break;
			default:
				uk = 0;
		}
	}

	int idx = uk / dbConfig->dbDiv % dbConfig->dbMod;

	int machineId = groupMap[idx];
	if(machineId == GMAP_NONE)
		return NULL;
	if(machineId == GMAP_DUMMY)
		return GROUP_DUMMY;

	CHelperGroup **ptr = &groups[machineId * GROUPS_PER_MACHINE];

	if(task->RequestCode()==DRequest::Get && ptr[GROUPS_PER_ROLE] &&
			false == guard->InSet(task->BarrierKey(), task->PackedKey()))
	{
		int role = 0;
		switch(dbConfig->mach[machineId].mode)
		{
			case BY_SLAVE:
				role = 1;
				break;

			case BY_DB:
				role = (uk / dbConfig->dbDiv) & 1;

			case BY_TABLE:
				role = (uk / dbConfig->tblDiv) & 1;

			case BY_KEY:
				role = task->BarrierKey() & 1;
		}

		return ptr[ role*GROUPS_PER_ROLE ];
	}

	int g = task->RequestType();

	while(--g >= 0) {
		if(ptr[g] != NULL) {
			return ptr[g];
		}
	}
	return NULL;
}

bool CGroupCollect::IsCommitFull (CTaskRequest *task)
{
	if(task->RequestCode() != DRequest::Replace)
		return false;

	CHelperGroup *helperGroup = SelectGroup(task);
	if (helperGroup==NULL || helperGroup==GROUP_DUMMY)
		return false;

	if(helperGroup->QueueFull()) {
		log_warning("NO FREE COMMIT QUEUE SLOT");
		helperGroup->DumpState();
	}
	return helperGroup->QueueFull () ? true : false;
}

void CGroupCollect::TaskNotify (CTaskRequest *task)
{
	CHelperGroup *helperGroup = SelectGroup(task);

	if (helperGroup == NULL) {
		log_error("Key not belong to this server");
		task->SetError (-EC_OUT_OF_KEY_RANGE, "CGroupCollect::TaskNotify", "Key not belong to this server");
		task->ReplyNotify();
	} else if(helperGroup == GROUP_DUMMY) {
		task->MarkAsBlackHole();
		task->ReplyNotify();
	} else {
		if(task->RequestType()==TaskTypeWrite && guardReply != NULL)
			task->PushReplyDispatcher(guardReply);
		helperGroup->TaskNotify (task);
	}

	StatHelperGroupQueueCount(groups, dbConfig->machineCnt * GROUPS_PER_MACHINE);
	StatHelperGroupCurMaxQueueCount(task->RequestType());
}

int CGroupCollect::LoadConfig (const CDbConfig *cfg, int keysize)
{
	dbConfig = cfg;

	groupMap = (short *)MALLOC(sizeof (short ) * dbConfig->dbMax);
	for(int i=0; i<dbConfig->dbMax; i++)
		groupMap[i] = GMAP_NONE;

	groups = (CHelperGroup**)CALLOC(sizeof (CHelperGroup*), dbConfig->machineCnt * GROUPS_PER_MACHINE);
	if (!groups || !groupMap) {
		log_error("malloc failed, %m");
		return -1;
	}

	/* build master group mapping */
	for (int i = 0; i < dbConfig->machineCnt; i++) {
		int gm_id = i;
		if(dbConfig->mach[i].helperType == DUMMY_HELPER) {
			gm_id = GMAP_DUMMY;
			hasDummyMachine = 1;
		} else if(dbConfig->mach[i].procs == 0) {
			continue;
		}
		for (int j = 0; j < dbConfig->mach[i].dbCnt; j++)
		{
			const int db = dbConfig->mach[i].dbIdx[j];
			if(groupMap[db] >= 0) {
				log_error("duplicate machine, db %d machine %d %d",
					db, groupMap[db]+1, i+1);
				return -1;
			}
			groupMap[db] = gm_id;
		}
	}

	/* build helper object */
	for (int i = 0; i < dbConfig->machineCnt; i++) {
		if(dbConfig->mach[i].helperType == DUMMY_HELPER)
			continue;
		for(int j=0; j<GROUPS_PER_MACHINE; j++) {
			if(dbConfig->mach[i].gprocs[j]==0) continue;

			char name[24];
			snprintf(name, sizeof(name), "%d%c%d", i, MACHINEROLESTRING[j/GROUPS_PER_ROLE], j%GROUPS_PER_ROLE);
			groups[i*GROUPS_PER_MACHINE+j] =
				new CHelperGroup (
				dbConfig->mach[i].role[j/GROUPS_PER_ROLE].path,
				name,
				dbConfig->mach[i].gprocs[j],
				dbConfig->mach[i].gqueues[j],
				TTC_SQL_USEC_ALL);
			if(j >= GROUPS_PER_ROLE)
				groups[i*GROUPS_PER_MACHINE+j]->fallback =
					groups[i*GROUPS_PER_MACHINE];
		}
	}

	if(dbConfig->slaveGuard) {
		guard = new CKeyGuard(keysize, dbConfig->slaveGuard);
		guardReply = new CGuardNotify(this);
	}
	return 0;
}


int CGroupCollect::Attach (CPollThread *thread)
{
	CTaskDispatcher<CTaskRequest>::AttachThread(thread);
	for (int i = 0; i < dbConfig->machineCnt * GROUPS_PER_MACHINE; i++)
	{
		if(groups[i])
			groups[i]->Attach(thread, &taskQueueAllocator);
	}
	return 0;
}

void CGroupCollect::SetTimerHandler(CTimerList *recv, CTimerList *conn, CTimerList *retry)
{
	for (int i = 0; i < dbConfig->machineCnt; i++) {
		if(dbConfig->mach[i].helperType == DUMMY_HELPER)
			continue;
		for(int j=0; j<GROUPS_PER_MACHINE; j++) {
			if(dbConfig->mach[i].gprocs[j]==0)
				continue;
			if(groups[i*GROUPS_PER_MACHINE+j])
				groups[i*GROUPS_PER_MACHINE+j]->SetTimerHandler(recv, conn, retry);
		}
	}
}

int CGroupCollect::DisableCommitGroup(void)
{
	if (groups == NULL)
		return 0;
	for (int i = 2;
		i < dbConfig->machineCnt * GROUPS_PER_MACHINE;
		i += GROUPS_PER_MACHINE)
	{
		DELETE(groups[i]);
	}
	return 0;
}

void CGroupCollect::StatHelperGroupQueueCount(CHelperGroup **groups, unsigned group_count)
{
	unsigned total_queue_count     = 0;
	unsigned total_queue_max_count = 0;

	for(unsigned i =0; i<group_count;++i)
	{
		if(groups[i])
		{
			total_queue_count 	+= groups[i]->QueueCount();
			total_queue_max_count	+= groups[i]->QueueMaxCount();
		}
	}

	statQueueCurCount = total_queue_count;
	statQueueMaxCount = total_queue_max_count;
	return;
}

int CGroupCollect::GetQueueCurMaxCount(int iColumn)
{
	int maxcount = 0;
	if ( (iColumn < 0)
		|| (iColumn >= GROUPS_PER_MACHINE) )
	{
		return maxcount;
	}
	
	for (int row = 0; row < dbConfig->machineCnt; row++)
	{
		/*read组是在group矩阵的第一列*/
		CHelperGroup *readGroup = groups[dbConfig->machineCnt * row + iColumn];
		if (NULL == readGroup)
		{
			continue;
		}
		if (readGroup->QueueCount() > maxcount)
		{
			
			maxcount = readGroup->QueueCount();
			log_debug("the group queue maxcount is %d ",maxcount );
		}
	}
	return maxcount;
}
/*传入请求类型，每次只根据请求类型统计响应的值*/
void CGroupCollect::StatHelperGroupCurMaxQueueCount(int iRequestType)
{
	/*根据请求类型分辨不出来是主读还是备读(和Workload配置有关)，只好同时即统计主读组又统计备读组了*/
	/*除非遍历group矩阵里的指针值和selectgroup后的group指针比较，然后再对比矩阵列，这个更复杂*/
	if (TaskTypeRead == iRequestType)
	{
		statReadQueueCurMaxCount = GetQueueCurMaxCount(MASTER_READ_GROUP_COLUMN);
		statSlaveReadQueueMaxCount = GetQueueCurMaxCount(SLAVE_READ_GROUP_COLUMN);
	}
	if (TaskTypeWrite  == iRequestType)
	{
		statWriteQueueMaxCount = GetQueueCurMaxCount(MASTER_WRITE_GROUP_COLUMN);
		
	}
	
	if (TaskTypeCommit == iRequestType)
	{
		statCommitQueueCurMaxCount = GetQueueCurMaxCount(MASTER_COMMIT_GROUP_COLUMN);
		
	}
	
}

