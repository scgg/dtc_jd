#include "map_reduce.h"

CDispatcher::CDispatcher(CPollThread *o, int fd):CPollerObject(o) {
	netfd = fd;
	waiting = 0;
	EnableInput();
	AttachPoller();
}

CDispatcher::~CDispatcher() {
}

bool CDispatcher::Dispatch(CProberCmd *cmd) {
	int ret = write(netfd, &cmd, sizeof(cmd));
	log_info("dispatching worker: %lx", (long int)cmd);
	++waiting;
	if (ret != sizeof(cmd)) {
		log_error("dispatch cmd error");
		return false;
	}
	return true;
}

void CDispatcher::InputNotify(void) {
	CProberCmd *cmd;
	CHttpSession *session;
	int ret = read(netfd, &cmd, sizeof(cmd));
	log_info("dispatcher input: %lx", (long int)cmd);
	if (ret != sizeof(cmd))
		log_error("input notify for read cmd error");
	// may seg fault
	//cmd->GetSession()->Complete();
	session = cmd->GetSession();
	session->Complete();
	--waiting;
}

CWorker::CWorker(CPollThread *o ,int fd):CPollerObject(o) {
	netfd = fd;
	EnableInput();
	AttachPoller();
}

CWorker::~CWorker() {
}

void CWorker::InputNotify(void) {
	CProberCmd *cmd;
	int ret = read(netfd, &cmd, sizeof(cmd));
	log_info("worker input: %lx", (long int)cmd);
	if (ret != sizeof(cmd))
		log_error("input notify for read cmd error");
	cmd->Cal();
	ret = write(netfd, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		log_error("write to dispatcher error");
}
