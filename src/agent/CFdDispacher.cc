/*
 * CFdDispacher.cpp
 *
 *  Created on: 2014Äê8ÔÂ4ÈÕ
 *      Author: Jiansong
 */
#include "CFdDispacher.h"
#include "front_worker.h"
#include "dispacher_worker_type.h"

CFdDispacher::CFdDispacher() {
	// TODO Auto-generated constructor stub
}

CFdDispacher::~CFdDispacher() {
	// TODO Auto-generated destructor stub
}

void CFdDispacher::TaskNotify(BaseTask *task) {
	//CDispacherTask *task=(CDispacherTask *)btask;
	switch (task->GetCmd()) {
	case (DISPACHEREQUEST): {
		RequestTask *rt = static_cast<RequestTask *>(task);
		log_debug("the fd receive from queue is %d", rt->GetFd());
		CFrontWorker *worker = new CFrontWorker(rt->GetId(), rt->GetFd(),
				ownerThread, rt->GetModule(),
				rt->GetModule()->GetStatManager());
		if (NULL == worker) {
			log_error("no mem new CAGFrontWorker");
			return;
		}
		rt->GetModule()->AddFrontWorker(ownerThread, worker);
		break;
	}
	case (CLOSEALLFRONTWORKERS): {
		CloseFwsTask *cft = static_cast<CloseFwsTask *>(task);
		cft->GetModule()->ReleseAllFrontWorker(ownerThread);
		break;
	}
	case (ADDCACHESERVER): {
		AddServerTask *ast = static_cast<AddServerTask *>(task);
		log_debug("enter CFdDispacher::TaskNotify:%s,%s,%d",
				ast->GetServerName().c_str(), ast->GetAddress().c_str(),
				ast->GetVirtualNode());
		ast->GetModule()->AddCacheServer(ownerThread, ast->GetServerName(),
				ast->GetAddress(), ast->GetVirtualNode(),
				ast->GetHotBackupAddress());
		break;
	}
	case (REMOVECACHESERVER): {
		RemoveServerTask *rst = static_cast<RemoveServerTask *>(task);
		rst->GetModule()->RemoveCacheServer(ownerThread, rst->GetServerName(),
				rst->GetVirtualNode());
		break;
	}
	case (RELASEALLCACHESERVERS): {
		RelaseAllServerTask *rst = static_cast<RelaseAllServerTask *>(task);
		rst->GetModule()->RelaseAllCacheServer(ownerThread);
		break;
	}
	case (AGENTPING): {
		break;
	}
	case (SWITCHCACHESERVER): {
		SwitchCacheServerTask *stask =static_cast<SwitchCacheServerTask *>(task);
		stask->GetModule()->SwitchCacheServer(ownerThread,stask->GetName(),stask->GetMode());
		break;
	}
	default: {
		log_debug("cmd error");
	}
	}

}

void CFdDispacher::SetOwnerThread(CPollThread *thread) {
	this->ownerThread = thread;
}
