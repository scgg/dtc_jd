/*
 * =====================================================================================
 *
 *       Filename:  client_resource_pool.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2010 06:09:53 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  newmanwang (nmwang), newmanwang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#ifndef CLIENT_RESOURCE_POOL_H
#define CLIENT_RESOURCE_POOL_H

#include <stdint.h>

class CTaskRequest;
class CPacket;
class CClientResourceSlot{
    public:
	CClientResourceSlot():
	    freenext(0),
	    freeprev(0),
	    task(NULL),
	    packet(NULL),
	    seq(0)
        {}
	~CClientResourceSlot();
	int freenext;
	/*only used if prev slot free too*/
	int freeprev;
	CTaskRequest * task;
	CPacket * packet;
	uint32_t seq;
};

/* newman: pool */
/* automaticaly change size according to usage status */
/* resource from pool need */
class CClientResourcePool{
    public:

	CClientResourcePool():
	    total(0),
	    used(0),
	    freehead(0),
	    freetail(0),
	    taskslot(NULL)
	{}
	~CClientResourcePool();
	int Init();
	inline CClientResourceSlot * Slot(unsigned int id){return &taskslot[id];}
	/* clean resource allocated */
	int Alloc(unsigned int & id, uint32_t & seq);
	int Fill(unsigned int id);
	void Clean(unsigned int id);
	/* free, should clean first */
	void Free(unsigned int id, uint32_t seq);

    private:
	unsigned int total;
	unsigned int used;
	unsigned int freehead;
	unsigned int freetail;
	CClientResourceSlot * taskslot;
    private:
	int Enlarge();
	void Shrink();
	inline int Half_use()
	{
	    return used <= total / 2 ? 1 : 0;
	}
};

#endif
