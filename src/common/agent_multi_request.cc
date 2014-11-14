#include "agent_multi_request.h"
#include "task_request.h"
#include "agent_client.h"

extern CTableDefinition *gTableDef[];

CAgentMultiRequest::CAgentMultiRequest(CTaskRequest * o):
	packetCnt(0),
	owner(o),
	taskList(NULL),
	compleTask(0),
	ownerClient(NULL)
{
    if(o)
        ownerClient = o->OwnerClient();
}


CAgentMultiRequest::~CAgentMultiRequest()
{
    ListDel();

    if(taskList)
    {
	for(int i = 0; i < packetCnt; i++)
	    if(taskList[i].task)
	        delete taskList[i].task;

	delete[] taskList;
    }

    if(!!packets)
	free(packets.ptr);
}

void CAgentMultiRequest::CompleteTask(int index)
{
    if(taskList[index].task)
    {
	delete taskList[index].task;
	taskList[index].task = NULL;
    }

    compleTask++;
    /* delete owner taskrequest along with us if all sub request's result putted into ClientAgent's send queue */

    if(compleTask == packetCnt)
    {
	delete owner;
    }
}

void CAgentMultiRequest::ClearOwnerInfo()
{
    ownerClient = NULL;

    if(taskList == NULL)
	return;

    for(int i = 0; i < packetCnt; i++)
    {
        if(taskList[i].task) 
	    taskList[i].task->ClearOwnerClient();
    }
}

/*
error case: set this task processed
1. no mem: set task processed
2. decode error: set task processed, reply this task
*/
void CAgentMultiRequest::DecodeOneRequest(char * packetstart, int packetlen, int index)
{
	int err = 0;
	CTaskRequest * task = NULL;
        CDecodeResult decoderes;

	task = new CTaskRequest(gTableDef[0]);
	if(NULL == task)
	{
		log_crit("not enough mem for new task creation, client wont recv response");
		compleTask++;
		return;
	}

        task->SetHotbackupTable(gTableDef[1]);
        decoderes = task->Decode(packetstart, packetlen, 2);
        switch(decoderes)
        {
        default:
        case DecodeFatalError:
            if(errno != 0)
                log_notice("decode fatal error, msg = %m");
            break;
        case DecodeDataError:
            task->ResponseTimerStart();
            task->MarkAsHit();
            taskList[index].processed = 1;
            break;
        case DecodeDone:
            if((err = task->PrepareProcess()) < 0)
            {
                log_error("build packed key error: %d, %s", err, task->resultInfo.ErrorMessage());
                taskList[index].processed = 1;
            }
            break;
        }

        task->SetOwnerInfo(this, index, NULL);
        task->SetOwnerClient(this->ownerClient);

        taskList[index].task = task;

        return;
}

int CAgentMultiRequest::DecodeAgentRequest()
{
    int cursor = 0;

    taskList = new DecodedTask[packetCnt];
    if(NULL == taskList)
    {
	log_crit("no mem new taskList");
	return -1;
    }
    memset((void *)taskList, 0, sizeof(DecodedTask) * packetCnt);

    /* whether can work, reply on input buffer's correctness */
    for(int i = 0; i < packetCnt; i++)
    {
        char * packetstart;
        int packetlen;

        packetstart = packets.ptr + cursor;
        packetlen = PacketBodyLen(*(CPacketHeader *)packetstart) + sizeof(CPacketHeader);

	DecodeOneRequest(packetstart, packetlen, i);
	
        cursor += packetlen;         
    }

    return 0;
}

void CAgentMultiRequest::CopyReplyForSubTask()
{
    for(int i = 0; i < packetCnt; i++)
    {
	if(taskList[i].task)
	    taskList[i].task->CopyReplyPath(owner);
    }
}
