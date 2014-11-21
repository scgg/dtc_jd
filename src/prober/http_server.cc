#include "http_server.h"
#include <errno.h>
#include <netinet/tcp.h>
#include <stdlib.h>

int targetNewHash;
int hashChanging;
CConfig* gConfig = NULL;
CDbConfig* gDbConfig = NULL;
CTableDefinition* gTableDef = NULL;
std::queue<CHttpSession *> CHttpSession::sessionQueue;

int CHttpAcceptor::ListenOn(const std::string &addr)
{
	const char * errmsg = NULL;

	errmsg = bindAddr.SetAddress(addr.c_str(), (const char *)NULL);
	if(errmsg)
	{
		log_error("%s %s", errmsg, addr.c_str());
		return -1;
	}

	netfd = bindAddr.CreateSocket();
	if(netfd < 0)
	{
		log_error("acceptor create socket failed: %m, %d", errno);
		return -1;
	}

	int optval = 1;
	setsockopt(netfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	setsockopt(netfd, SOL_TCP, TCP_NODELAY, &optval, sizeof(optval));
	optval = 60;
	setsockopt(netfd, SOL_TCP, TCP_DEFER_ACCEPT, &optval, sizeof(optval));

	if(bindAddr.BindSocket(netfd) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor bind address failed: %m, %d", errno);
		return -1;
	}

	if(bindAddr.SocketType() == SOCK_STREAM && listen(netfd, 255) < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor listen failed: %m, %d", errno);
		return -1;
	}

	EnableInput();

	if(AttachPoller() < 0)
	{
		close(netfd);
		netfd = -1;
		log_error("acceptor attach listen socket failed");
		return -1;
	}

	return 0;
}

void CHttpAcceptor::InputNotify()
{
	int newfd;
	struct sockaddr peer;
	socklen_t peerSize = sizeof(peer);

	newfd = accept(netfd, (struct sockaddr *)&peer, &peerSize);
	if(newfd < 0)
	{
		if(errno != EINTR && errno != EAGAIN)
			log_notice("accept new client error: %m, %d", errno);
		return;
	}

	CHttpSession *worker = new CHttpSession(pollThread, newfd, proberConfig, dp);

	if(NULL == worker)
	{
		log_error("no mem new CHttpWorker");
		return;
	}
}

bool CHttpSession::ProcessBody(std::string reqBody, std::string &respBody) {
	bool ret = true;
	std::string cn;

	if (!reader.parse(reqBody, in, false)) {
		prs.retcode = RET_CODE_INTERNAL_ERR;
		prs.errmsg = "解析html请求出错";
		ret = false;
		log_error("parse body error. requst body: %s", reqBody.c_str());
		goto out;
	}
	cn = proberConfig->CmdCodeToClassName(in["cmdcode"].asString());
	log_debug("cmd code: %s, class name: %s", in["cmdcode"].asString().c_str(), cn.c_str());
	if (!(std::string("") != cn && (proberCmd = CProberCmd::CreatConcreteCmd(cn)) != NULL)) {
		prs.retcode = RET_CODE_NO_CMD;
		prs.errmsg = "当前版本探针服务不支持此命令";
		ret = false;
		log_error("cmdcode: %s not support", in["cmdcode"].asString().c_str());
		goto out;
	}
	log_debug("proberCmd: %lx", (long int)proberCmd);
	cmdString = cn;
	if (atoi(in["cmdcode"].asString().c_str()) > MEM_CMD_BASE) {
		// queued
		log_info("sessionQueue.size(): %ld", sessionQueue.size());
		if (sessionQueue.size() >= proberConfig->QueueSize()) {
			prs.retcode = RET_CODE_BUSY;
			prs.errmsg = "探针服务忙，请稍后再试";
			log_error("queue size exceed");
			ret = false;
			goto out;
		} else if (sessionQueue.size() > 0) {
			sessionQueue.push(this);
			return true;
		} else {
			sessionQueue.push(this);
			CHttpSession *cur = sessionQueue.front();
			return cur->ProcessQueued();
		}
	}

	// we reach here if we are doing stat cmd
	ret = proberCmd->ProcessCmd(in, prs);

out:
	DELETE(proberCmd);
	out["retcode"] = prs.retcode;
	out["errmsg"] = prs.errmsg;
	out["resp"] = prs.resp;
	respBody = writer.write(out);
	ReplyPkg();

	return ret;
}

bool CHttpSession::ProcessQueued() {
	if (!InitMemConf(std::string("/usr/local/dtc/") + in["accesskey"].asString() + "/conf")) {
		prs.retcode = RET_CODE_INTT_MEM_CONF_ERR;
		prs.errmsg = "打开缓存配置出错";
		log_error("init mem conf error, %s", in["accesskey"].asString().c_str());
		goto out;
	}
	if (!proberCmd->InitAndCheck(in, prs))
		goto out;
	DELETE(gConfig);
	if (NULL != gDbConfig) {
		gDbConfig->Destroy();
		gDbConfig = NULL;
	}
	DELETE(gTableDef);
	proberCmd->SetSession(this);
	cmdArray = new CProberCmd*[targetCmd];
	cmdArray[0] = proberCmd;
	for (int i = 1; i < targetCmd; ++i) {
		cmdArray[i] = CProberCmd::CreatConcreteCmd(cmdString);
		cmdArray[i]->AttachMem(proberCmd);
		cmdArray[i]->SetSession(this);
	}
	if (in["content"].isObject() && in["content"]["noempty"].asInt() == PROBER_NO_EMPTY) {
		log_debug("no empty node");
		for (int i = 0; i < targetCmd; ++i)
			cmdArray[i]->SetNoEmpty();
	}
	return Split();
out:
	DELETE(proberCmd);
	DELETE(gConfig);
	if (NULL != gDbConfig) {
		gDbConfig->Destroy();
		gDbConfig = NULL;
	}
	DELETE(gTableDef);
	out["retcode"] = prs.retcode;
	out["errmsg"] = prs.errmsg;
	out["resp"] = prs.resp;
	respBody = writer.write(out);
	ReplyPkg();
	log_error("ProcessQueued error, sessionQueue.size(): %ld", sessionQueue.size());
	sessionQueue.pop();
	if (sessionQueue.size() > 0) {
		CHttpSession *cur = sessionQueue.front();
		cur->ProcessQueued();
	}

	return false;
	
}

bool CHttpSession::Split() {
	int min = cmdArray[0]->MinNodeId();
	int everyWorker = (cmdArray[0]->MaxNodeId() - min) / targetCmd;
	log_debug("min node: %d, max node: %d", cmdArray[0]->MinNodeId(), cmdArray[0]->MaxNodeId());
	for (int i = 0; i < targetCmd; ++i) {
		cmdArray[i]->SetInterval(min + i * everyWorker, min + (i + 1) * everyWorker);
		dp[i]->Dispatch(cmdArray[i]);
	}
	return true;
}

bool CHttpSession::InitMemConf(std::string confBase) {
	log_debug("InitMemConf with %s", confBase.c_str());
	std::string cacheCfg = confBase + "/cache.conf";
	std::string tableCfg = confBase + "/table.conf";
	gConfig = new CConfig;
	if(gConfig->ParseConfig(cacheCfg.c_str(), "cache")){
		log_error("parse cache config error");
		return false;
	}
	hashChanging = gConfig->GetIntVal("cache", "HashChanging", 0);
	targetNewHash = gConfig->GetIntVal("cache", "TargetNewHash", 0);
	//TODO global variable problem
	RELATIVE_HOUR_CALCULATOR->SetBaseHour(gConfig->GetIntVal("cache", "RelativeYear", 2014));

	gDbConfig = CDbConfig::Load(tableCfg.c_str());
	if(gDbConfig == NULL){
		log_error("load table configuire file error");
		return false;
	}

	gTableDef = gDbConfig->BuildTableDefinition();
	if(gTableDef == NULL){
		log_error("build table definition error");
		return false;
	}
	return true;
}

int CHttpSession::HandlePkg(void *pPkg, int iPkgLen)
{
	log_debug("handle package:%s pkglen:%d", (char *)pPkg, iPkgLen);
	if (decoder.Decode((char *)pPkg, iPkgLen) < 0)
	{
		CloseConnection();
		log_error("Decode Error");
	}
	else
	{
		//process message
		ProcessBody(decoder.GetBody(), respBody);
	}

	return 0;
}

void CHttpSession::Complete() {
	if (++doneCmd != targetCmd)
		return;
	for (int i = 1; i < targetCmd; ++i) {
		cmdArray[0]->Combine(cmdArray[i]);
		cmdArray[i]->DetachMem();
	}
	cmdArray[0]->ReplyBody(respBody);
	ReplyPkg();
	for (int i = 0; i < targetCmd; ++i)
		delete cmdArray[i];
	delete [] cmdArray;
	cmdArray = NULL;
	proberCmd = NULL;
	sessionQueue.pop();
	log_info("sessionQueue.size(): %ld", sessionQueue.size());
	if (sessionQueue.size() > 0) {
		CHttpSession *cur = sessionQueue.front();
		cur->ProcessQueued();
	}
}

void CHttpSession::ReplyPkg() {
	encoder.SetCode(200);
	encoder.SetHttpVersion(0);
	encoder.SetKeepAlive(false);
	encoder.SetBody(respBody);
	std::string response = encoder.Encode();
	//send response
	char *send = (char*)malloc(response.size()*sizeof(char));
	memcpy(send, response.c_str(), response.size());
	agentSender->AddPacket(send, response.size());
	//agentSender->AddPacket(response.c_str(), response.size());
	agentSender->Send();
	if (!decoder.IsKeepAlive())
		CloseConnection();
	else
		AttachTimer(timerList);
	return;
}
