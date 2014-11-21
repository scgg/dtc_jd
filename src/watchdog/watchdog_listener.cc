#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "watchdog_listener.h"

CWatchDogListener::CWatchDogListener(CWatchDog *o) : owner(o), peerfd(-1)
{
};

CWatchDogListener::~CWatchDogListener(void)
{
	if(peerfd > 0)
		close(peerfd);
};

int CWatchDogListener::AttachWatchDog(void)
{
	int fd[2];
	socketpair(AF_UNIX, SOCK_DGRAM, 0, fd);
	netfd = fd[0];
	peerfd = fd[1];

	char buf[30];
	snprintf(buf, sizeof(buf), "%d", peerfd);
	setenv(ENV_WATCHDOG_SOCKET_FD, buf, 1);

	int no;
	setsockopt(netfd, SOL_SOCKET, SO_PASSCRED, (no=1,&no), sizeof(int));
	fcntl(netfd, F_SETFD, FD_CLOEXEC);
	EnableInput();
	owner->SetListener(this);
	return 0;
}

void CWatchDogListener::InputNotify(void)
{
	char buf[16];
	int n;
	struct msghdr msg = { 0 };
	char ucbuf[CMSG_SPACE(sizeof(struct ucred))];
	struct iovec iov[1];

	iov[0].iov_base = (void *)&buf;
	iov[0].iov_len  = sizeof(buf);
	msg.msg_control = ucbuf;
	msg.msg_controllen = sizeof ucbuf;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	while((n=recvmsg(netfd, &msg, MSG_TRUNC|MSG_DONTWAIT)) > 0)
	{
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
		struct ucred *uc;
		if( msg.msg_controllen < sizeof(ucbuf) ||
				cmsg->cmsg_level != SOL_SOCKET ||
				cmsg->cmsg_type != SCM_CREDENTIALS ||
				cmsg->cmsg_len != CMSG_LEN(sizeof(struct ucred)) ||
				msg.msg_controllen < cmsg->cmsg_len)
			continue;
		uc = (struct ucred *)CMSG_DATA(cmsg);
		if(n!=sizeof(buf))
			continue;

		CWatchDogObject *obj = new CWatchDogObject(owner, buf, uc->pid);
		obj->AttachWatchDog();
	}
}

