#include <stdio.h>
#include <stdlib.h>

#include "client_resource_pool.h"
#include "packet.h"
#include "task_request.h"
#include "log.h"

CClientResourceSlot::~CClientResourceSlot()
{
    DELETE(task);
    DELETE(packet);
}

CClientResourcePool::~CClientResourcePool()
{
    /* free resource pool when thread exit */
    /*
    if(taskslot)
    {
        for(unsigned int i = 0; i < total; i ++)
        {
            taskslot[i].~CClientResourceSlot();
        }
        FREE_IF(taskslot);
    }
    */
}

/* newman: pool*/
/* rules: */
/* head free slot: freeprev == 0
 * tail free slot: freenext == 0
 * used slot: freeprev == 0, freenext == 0
 * non-empty list: freehead, freetail not 0
 * empty list: freehead, freetail == 0
 * */
int CClientResourcePool::Init()
{
    taskslot = (CClientResourceSlot *)CALLOC(256, sizeof(CClientResourceSlot));
    if(taskslot == NULL)
	return -1;

    total = 256;
    used = 1;
    freehead = 1;
    freetail = total - 1;

    /* 1(freehead), 2..., total - 1(freetail) */
    taskslot[1].freenext = 2;
    taskslot[1].freeprev = 0;
    if((taskslot[1].task = new CTaskRequest) == NULL)
	return -1;
    if((taskslot[1].packet = new CPacket) == NULL)
	return -1;
    taskslot[1].seq = 0;

    for(unsigned int i = 2; i < total - 1; i ++)
    {
	taskslot[i].freenext = i + 1;
	taskslot[i].freeprev = i - 1;
	if((taskslot[i].task = new CTaskRequest) == NULL)
	    return -1;
	if((taskslot[i].packet = new CPacket) == NULL)
	    return -1;
	taskslot[i].seq = 0;
    }

    taskslot[total - 1].freenext = 0;
    taskslot[total - 1].freeprev = total - 2;
    if((taskslot[total - 1].task = new CTaskRequest) == NULL)
	return -1;
    if((taskslot[total - 1].packet = new CPacket) == NULL)
	return -1;
    taskslot[total - 1].seq = 0;

    return 0;
}

int CClientResourcePool::Alloc(unsigned int & id, uint32_t &seq)
{
    if(freehead == 0)
    {
        if(Enlarge() < 0)
            return -1;
    }

    used ++;
    id = freehead;
    freehead = taskslot[id].freenext;
    if(freehead == 0)
	    freetail = 0;
    taskslot[id].freenext = 0;
    if(freetail != 0)
        taskslot[freehead].freeprev = 0;

    
    if(taskslot[id].task == NULL)
    {
        if((taskslot[id].task = new CTaskRequest) == NULL)
        {
            used --;
            if(freetail == 0)
                freetail = id;
            taskslot[id].freenext = freehead;
            taskslot[freehead].freeprev = id;
            freehead = id;
            return -1;
        }
    }
    if(taskslot[id].packet == NULL)
    {
        if((taskslot[id].packet = new CPacket) == NULL)
        {
            used --;
            if(freetail == 0)
                freetail = id;
            taskslot[id].freenext = freehead;
            taskslot[freehead].freeprev = id;
            freehead = id;
            return -1;
        }
    }
    
    taskslot[id].seq++;
    seq = taskslot[id].seq;

    return 0;
}

void CClientResourcePool::Clean(unsigned int id)
{
    if(taskslot[id].task)
        taskslot[id].task->Clean();
    if(taskslot[id].packet)
        taskslot[id].packet->Clean();
}

void CClientResourcePool::Free(unsigned int id, uint32_t seq)
{
    if(taskslot[id].seq != seq)
    {
    	log_crit("client resource seq not right: %u, %u, not free", taskslot[id].seq, seq);
	return;
    }


    used --;
    taskslot[freehead].freeprev = id;
    taskslot[id].freenext = freehead;
    freehead = id;
    if(freetail == 0)
	freetail = id;

//    if(Half_use())
//	Shrink();
}

int CClientResourcePool::Fill(unsigned int id)
{
    if(taskslot[id].task && taskslot[id].packet)
	return 0;
   
    if((taskslot[id].task == NULL) && ((taskslot[id].task = new CTaskRequest) == NULL))
	return -1;
    if((taskslot[id].packet == NULL) && ((taskslot[id].packet = new CPacket) == NULL))
	return -1;

    return 0;
}

/* enlarge when freehead == 0, empty list */
int CClientResourcePool::Enlarge()
{
    void * p;
    unsigned int old = total;

    p = REALLOC(taskslot, sizeof(CClientResourceSlot) * 2 * total);
    if(!p)
    {
	log_warning("enlarge taskslot failed, current CTaskRequestSlot %u", total);
	return -1;
    }

    total *= 2;
    freehead = old;
    freetail = total - 1;

    taskslot[old].freenext = old + 1;
    taskslot[old].freeprev = 0;
    if((taskslot[old].task = new CTaskRequest) == NULL)
    {
	log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	return -1;
    }
    if((taskslot[old].packet = new CPacket) == NULL)
    {
	log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	return -1;
    }
    taskslot[old].seq = 0;

    for(unsigned int i = old + 1; i < total - 1; i ++)
    {
	taskslot[i].freenext = i + 1;
	taskslot[i].freeprev = i - 1;
	if((taskslot[i].task = new CTaskRequest) == NULL)
	{
	    log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	    return -1;
	}
	if((taskslot[i].packet = new CPacket) == NULL)
	{
	    log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	    return -1;
	}
	taskslot[i].seq = 0;
    }

    taskslot[total - 1].freenext = 0;
    taskslot[total - 1].freeprev = total - 2;
    if(!(taskslot[total - 1].task = new CTaskRequest))
    {
	log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	return -1;
    }
    if(!(taskslot[total - 1].packet = new CPacket))
    {
	log_warning("no mem for new task slot, current CTaskRequestSlot %u", total);
	return -1;
    }
    taskslot[total - 1].seq = 0;

    return 0;
}

void CClientResourcePool::Shrink()
{
    int shrink_cnt = (total - used) / 2;
    int slot = freetail;

    if(freetail == 0)
	return;

    for(int i = 0; i < shrink_cnt; i ++)
    {
	if(taskslot[slot].task)
	{
	    delete taskslot[slot].task;
	    taskslot[slot].task = NULL;
	}
	if(taskslot[slot].packet)
	{
	    delete taskslot[slot].packet;
	    taskslot[slot].packet = NULL;
	}

	slot = taskslot[slot].freeprev;
	if(slot == 0)
	    break;
    }
}

