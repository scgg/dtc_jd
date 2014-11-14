#include "fd_transfer.h"
#include <unistd.h>
#include "log.h"
#include "poll_thread.h"
#include "poll_thread_group.h"
#include "session.h"

CFDTransfer::CFDTransfer(){
	int fd[2];
	int ret = pipe(fd);
	if (ret != 0)
		throw;
	netfd = fd[0];
	write_fd = fd[1];
}

int CFDTransfer::AttachPoller(CPollThread *thread){
	poll_thread = thread;
	EnableInput();
	return CPollerObject::AttachPoller(thread);
}

void CFDTransfer::InputNotify(){
	int fd;
	int ret = read(netfd, &fd, sizeof(int));
	if (ret != sizeof(int)){
		log_error("read fd fail");
		return;
	}
	Session *session = new Session(poll_thread, fd);
	if (session == NULL){
		log_error("no mem to new Session");
	}
}

int CFDTransfer::Transfer(int fd){
	int ret = write(write_fd, &fd, sizeof(int));
	if (ret != sizeof(int)){
		log_error("write fd fail, fd is %d", fd);
		return -1;
	}
	return 0;
}

CFDTransferGroup::CFDTransferGroup(int num){
	group_count = num;
	transfer_group = new CFDTransfer[group_count];
}

void CFDTransferGroup::Attach(CPollThreadGroup *pollGroup){
	for(int i=0; i<group_count; i++){
		transfer_group[i].AttachPoller(pollGroup->GetPollThread());
	}
}

void CFDTransferGroup::Transfer(int fd){
	static int count = 0;
	int index = count%group_count;
	count++;
	transfer_group[index].Transfer(fd);
}
