#include <stdio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "client_dgram.h"
#include "task_request.h"
#include "client_unit.h"
#include "protocol.h"
#include "log.h"

class CPollThread;
static int GetSocketFamily(int fd)
{
	struct sockaddr addr;
	bzero(&addr, sizeof(addr));
	socklen_t alen = sizeof(addr);
	getsockname(fd, &addr, &alen);
	return addr.sa_family;
}

class CReplyDgram : public CReplyDispatcher<CTaskRequest>
{
public:
	CReplyDgram(void) {}
	virtual ~CReplyDgram(void);
	virtual void ReplyNotify(CTaskRequest *task);
};

CReplyDgram::~CReplyDgram(void) { }

void CReplyDgram::ReplyNotify(CTaskRequest *task)
{
	CDgramInfo *info = task->OwnerInfo<CDgramInfo>();
	if (info == NULL) {
		delete task;
	} else if(info->cli == NULL) {
		log_error("info->cli is NULL, possible memory corrupted");
		FREE(info);
		delete task;
	} else {
		info->cli->SendResult(task, info->addr, info->len);
		FREE(info);
	}
}

static CReplyDgram replyDgram;

CClientDgram::CClientDgram(CTTCDecoderUnit *o, int fd)
	:CPollerObject (o->OwnerThread(), fd),
	owner(o),
	hastrunc(0),
	mru(0),
	mtu(0),
	alen(0),
	abuf(NULL)
{
}

CClientDgram::~CClientDgram ()
{    
	FREE_IF(abuf);
}

int CClientDgram::InitSocketInfo(void)
{
	switch(GetSocketFamily(netfd)) {
		default:
			mru = 65508;
			mtu = 65507;
			alen = sizeof(struct sockaddr);
			break;
		case AF_UNIX:
			mru = 16<<20;
			mtu = 16<<20;
			alen = sizeof(struct sockaddr_un);
			break;
		case AF_INET:
			hastrunc = 1;
			mru = 65508;
			mtu = 65507;
			alen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			hastrunc = 1;
			mru = 65508;
			mtu = 65507;
			alen = sizeof(struct sockaddr_in6);
			break;
	}
	return 0;
}

int CClientDgram::AllocateDgramInfo(void)
{
	if(abuf!=NULL)
		return 0;

	abuf = (CDgramInfo *)MALLOC(sizeof(CDgramInfo)+alen);
	if(abuf==NULL)
		return -1;

	abuf->cli = this;
	return 0;
}



int CClientDgram::Attach ()
{
	InitSocketInfo();

	EnableInput();
	if (AttachPoller () == -1)
		return -1;

	return 0;
}

// server peer
// return value: -1, fatal error or no more packets
// return value: 0, more packet may be pending
int CClientDgram::RecvRequest (int noempty)
{
	if(AllocateDgramInfo() < 0) {
		log_error("%s", "create CDgramInfo object failed, msg[no enough memory]");
		return -1;
	}

	int dataLen;
	if(hastrunc) {
		char dummy[1];
		abuf->len = alen;
		dataLen = recvfrom(netfd, dummy, 1, MSG_DONTWAIT|MSG_TRUNC|MSG_PEEK, (sockaddr *)abuf->addr, &abuf->len);
		if(dataLen < 0) {
			// NO ERROR, and assume NO packet pending
			return -1;
		}
	} else {
		dataLen = -1;
		ioctl(netfd, SIOCINQ, &dataLen);
		if(dataLen < 0 ) {
			log_error("%s", "next packet size unknown");
			return -1;
		}

		if(dataLen==0) {
			if(noempty) {
				// NO ERROR, and assume NO packet pending
				return -1;
			}
			/* treat 0 as max packet, because we can't ditiguish the no-packet & zero-packet */
			dataLen = mru;
		}
	}

	char *buf = (char *)MALLOC(dataLen);
	if(buf==NULL) {
		log_error("allocate packet buffer[%d] failed, msg[no enough memory]", dataLen);
		return -1;
	}

	abuf->len = alen;
	dataLen = recvfrom(netfd, buf, dataLen, MSG_DONTWAIT, (sockaddr *)abuf->addr, &abuf->len);
	if(abuf->len <= 1) {
		log_notice("recvfrom error: no source address");
		free(buf);
		// more packet pending
		return 0;
	}

	if(dataLen <= (int)sizeof(CPacketHeader))
	{
		int err = dataLen==-1 ? errno : 0;
		if(err != EAGAIN)
			log_notice("recvfrom error: size=%d errno=%d", dataLen, err);
		free(buf);
		// more packet pending
		return 0;
	}

	CTaskRequest *task = new CTaskRequest(owner->OwnerTable());
	if (NULL == task)
	{
		log_error("%s", "create CTaskRequest object failed, msg[no enough memory]");
		return -1;
	}

	task->SetHotbackupTable(owner->AdminTable());

	int ret = task->Decode (buf, dataLen, 1/*eat*/);
	switch (ret) 
	{
		default:
		case DecodeFatalError:
			if(errno != 0)
				log_notice("decode fatal error, ret = %d msg = %m", ret);
			free(buf); // buf not eatten
			break;

		case DecodeDataError:
			task->ResponseTimerStart();
			if(task->ResultCode() < 0)
				log_notice("DecodeDataError, role=%d, fd=%d, result=%d", 
					task->Role (), netfd, task->ResultCode ());
			SendResult(task, (void *)abuf->addr, abuf->len);
			break;

		case DecodeDone:
			if(task->PrepareProcess() < 0) {
				SendResult(task, (void *)abuf->addr, abuf->len);
				break;
			}

			task->SetOwnerInfo(abuf, 0, (sockaddr *)abuf->addr);
			abuf = NULL; // eat abuf

			task->PushReplyDispatcher(&replyDgram);
			owner->TaskDispatcher(task);
	}
	return 0;
}

int CClientDgram::SendResult (CTaskRequest *task, void *addr, int len)
{
	if(task == NULL)
		return 0;

	owner->RecordRequestTime(task);
	CPacket *reply = new CPacket;
	if(reply == NULL)
	{
		delete task;
		log_error("create CPacket object failed");
		return 0;
	}

	task->versionInfo.SetKeepAliveTimeout(owner->IdleTime());
	reply->EncodeResult (task, mtu);
	delete task;

	int ret = reply->SendTo(netfd, (struct sockaddr *)addr, len);

	delete reply;

	if (ret != SendResultDone)
	{
		log_notice("send failed, return = %d, error = %m", ret);
		return 0;
	}
	return 0;
}

void CClientDgram::InputNotify (void)
{
	const int batchsize = 64;
	for(int i=0; i<batchsize; ++i ) {
		if(RecvRequest (i) < 0)
			break;
	}
}

