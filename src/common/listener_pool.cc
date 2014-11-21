#include <stdlib.h>

#include "listener_pool.h"
#include "listener.h"
#include "client_unit.h"
#include "poll_thread.h"
#include "unix_socket.h"
#include "task_multiplexer.h"
#include "task_request.h"
#include "config.h"
#include "log.h"

extern CTableDefinition *gTableDef[];

CListenerPool::CListenerPool(void)
{
	memset(listener, 0, sizeof(listener));
	memset(thread, 0, sizeof(thread));
	memset(decoder, 0, sizeof(decoder));
}

int CListenerPool::InitDecoder(int n, int idle, CTaskDispatcher<CTaskRequest> *out)
{
		if(thread[n]==NULL)
		{
			char name[16];
			snprintf(name, sizeof(name), "inc%d", n);
			thread[n] = new CPollThread(name);
			if (thread[n]->InitializeThread() == -1)
				return -1;
			/* newman: pool */
			try{decoder[n] = new CTTCDecoderUnit(thread[n], gTableDef, idle);}
			catch(int err) {DELETE(decoder[n]); return -1;}
			if(decoder[n] == NULL)
			    return -1;
			
			CTaskMultiplexer* taskMultiplexer = new CTaskMultiplexer(thread[n]);
			taskMultiplexer->BindDispatcher(out);
			decoder[n]->BindDispatcher(taskMultiplexer);
		}
		
		return 0;
}

int CListenerPool::Bind(CConfig *gc, CTaskDispatcher<CTaskRequest> *out)
{
    bool hasBindAddr = false;
	int idle = gc->GetIntVal("cache", "IdleTimeout", 100);
	if(idle < 0){
		log_notice("IdleTimeout invalid, use default value: 100");
		idle = 100;
	}
	int single = gc->GetIntVal("cache", "SingleIncomingThread", 0);
	int backlog = gc->GetIntVal("cache", "MaxListenCount", 256);
	int win = gc->GetIntVal("cache", "MaxRequestWindow", 0);

	for(int i=0; i<MAXLISTENERS; i++)
	{
		const char *errmsg;
		char bindStr[32];
		char bindPort[32];
		int rbufsz;
		int wbufsz;

		if(i==0) {
			snprintf(bindStr, sizeof(bindStr), "BindAddr");
			snprintf(bindPort, sizeof(bindPort), "BindPort");
		} else {
			snprintf(bindStr, sizeof(bindStr), "BindAddr%d", i);
			snprintf(bindPort, sizeof(bindPort), "BindPort%d", i);
		}

		const char *addrStr = gc->GetStrVal("cache", bindStr);
		if(addrStr==NULL)
			continue;
		errmsg = sockaddr[i].SetAddress( addrStr, gc->GetStrVal("cache", bindPort));
		if(errmsg) {
			log_error("bad BindAddr%d/BindPort%d: %s\n", i, i, errmsg);
			continue;
		}

		int  n = single ? 0 : i;
		if(sockaddr[i].SocketType() == SOCK_DGRAM)
		{	// DGRAM listener
			rbufsz =  gc->GetIntVal("cache", "UdpRecvBufferSize",0);
			wbufsz =  gc->GetIntVal("cache", "UdpSendBufferSize",0);
		} else {
			// STREAM socket listener
			rbufsz = wbufsz = 0;
		}

		listener[i] = new CListener (&sockaddr[i]);
		listener[i]->SetRequestWindow(win);
		if(listener[i]->Bind(backlog, rbufsz, wbufsz) != 0){
			if(i == 0){
				log_crit("Error bind unix-socket");
				return -1;
			}
			else{
				continue;
			}
		}

		if(InitDecoder(n, idle, out) != 0)
			return -1;
		if(listener[i]->Attach(decoder[n], backlog) < 0)
			return -1;
        hasBindAddr = true;
	}
    if (!hasBindAddr)
    {
        log_crit("Must has a BindAddr");
        return -1;
    }

	for(int i=0; i<MAXLISTENERS; i++)
	{
		if(thread[i] == NULL)
			continue;
		thread[i]->RunningThread();
	}
	return 0;
}

CListenerPool::~CListenerPool(void)
{
	for(int i=0; i<MAXLISTENERS; i++)
	{
		if(thread[i])
		{
			thread[i]->interrupt();
			//delete thread[i];
		}
		DELETE(listener[i]);
		DELETE(decoder[i]);
	}
}

int CListenerPool::Match(const char *name, int port)
{
	for(int i=0; i<MAXLISTENERS; i++) {
		if(listener[i]==NULL)
			continue;
		if(sockaddr[i].Match(name, port))
			return 1;
	}
	return 0;
}

int CListenerPool::Match(const char *name, const char *port)
{
	for(int i=0; i<MAXLISTENERS; i++) {
		if(listener[i]==NULL)
			continue;
		if(sockaddr[i].Match(name, port))
			return 1;
	}
	return 0;
}

