#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "unix_socket.h"
#include "listener.h"
#include "poll_thread.h"
#include "decoder_base.h"
#include "log.h"
#include <StatTTC.h>

//static int statEnable=0;
//static CStatItemU32 statCurConnCount;
extern int gMaxConnCnt;

CListener::CListener (const CSocketAddress *s)
{
	sockaddr = s;
	bind = 0;
	window = 0;
	outPeer = NULL;
//	if(!statEnable){
//		statCurConnCount = statmgr.GetItemU32(CONN_COUNT);
//        statCurConnCount = 0;
//		statEnable = 1;
//	}
}

CListener::~CListener()
{
	if(sockaddr && sockaddr->SocketFamily()==AF_UNIX && netfd >= 0)
	{
		if(sockaddr->Name()[0]=='/')
		{
			unlink(sockaddr->Name());
		}
	}
	/* 在线连接数清0 */
	//statCurConnCount=0;
}

int CListener::Bind (int blog, int rbufsz, int wbufsz)
{
	if(bind)
		return 0;
		
	if ((netfd = SockBind (sockaddr, blog, rbufsz, wbufsz, 1/*reuse*/, 1/*nodelay*/, 1/*defer_accept*/)) == -1)
		return -1;

	bind = 1;
	
	return 0;
}

int CListener::Attach (CDecoderUnit *unit, int blog, int rbufsz, int wbufsz)
{
	if(Bind(blog, rbufsz, wbufsz) != 0)
		return -1;

	outPeer = unit;
	if(sockaddr->SocketType()==SOCK_DGRAM) {
		if(unit->ProcessDgram(netfd) < 0)
			return -1;
		else
			netfd = dup(netfd);
		return 0;
	}
	EnableInput();
	return AttachPoller(unit->OwnerThread());
}

void CListener::InputNotify (void)
{
	log_debug("enter InputNotify!!!!!!!!!!!!!!!!!");
	int newfd = -1;
	socklen_t peerSize;
	struct sockaddr peer;

	while (true)
	{
		peerSize = sizeof (peer);
		newfd = accept (netfd, &peer, &peerSize);

		if (newfd == -1)
		{
			if (errno != EINTR && errno != EAGAIN  )
				log_notice("[%s]accept failed, fd=%d, %m", sockaddr->Name(), netfd);

			break;
		}

//		if(statCurConnCount.get() >= gMaxConnCnt){
//				if (gMaxConnCnt == 0)//gMaxConnCnt may not init yet
//						continue;
//			log_error("too many connection, cur-connection-count[%u], close socket now. please enlarge `MaxFdCount`", (unsigned int)(statCurConnCount.get()));
//			close(newfd);
//
//			/* 向二级网管上报异常 */
//			log_alert("openning fd overflow[current=%u max=%u]", (unsigned)statCurConnCount, gMaxConnCnt);
//
//			continue;
//		}
		
		if(outPeer->ProcessStream(newfd, window, &peer, peerSize) < 0)
			close(newfd);

	}
}
