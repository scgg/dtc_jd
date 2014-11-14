#include "join.h"
#include "log.h"
#include "fd_transfer.h"
#include <errno.h>

Join::Join(const char *name, int listenfd, CFDTransferGroup *group) : CThread(name, CThread::ThreadTypeAsync),netfd(listenfd),fdtransfer_group(group)
{
}

Join::~Join()
{
}

void* Join::Process(void){
	while(false == Stopping()){
		int newfd;
		struct sockaddr peer;
		socklen_t peerSize = sizeof(peer);

		newfd = accept(netfd, (struct sockaddr *)&peer, &peerSize);
		if(newfd < 0){
			if(errno != EINTR && errno != EAGAIN)
				log_error("accept new client error: %m, %d", errno);
			continue;
		}
		//pass fd
		fdtransfer_group->Transfer(newfd);
	}
	return 0;
}
