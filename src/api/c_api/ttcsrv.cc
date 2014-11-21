#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <linux/sockios.h>
#include <pthread.h>


#include "unix_socket.h"
#include "ttcint.h"
#include <timestamp.h>
#include <container.h>
#include "curl_http.h"
#include "json/json.h"

using namespace std;

const uint32_t ACCESS_KEY_LEN = 40;

const char* ttc_api_ver="ttc-api-ver:4.3.4"; 
const char* ttc_api_complite="ttc-api-complite-date:"__DATE__" "__TIME__;

int NCServer::_network_mode = 0;
char * NCServer::_server_address = NULL;
char * NCServer::_server_tablename = NULL;
extern "C" void set_network_mode(void)
{
    NCServer::_network_mode = 1;
}

extern "C" void set_server_address(const char * address)
{
    NCServer::_server_address = strdup(address);
}

extern "C" void set_server_tablename(const char * tablename)
{
    NCServer::_server_tablename = strdup(tablename);
}

NCServer::NCServer(void)
{
	keytype = DField::None;

	tablename = NULL;
	appname = NULL;

	autoUpdateTable = 1;
	autoReconnect = 1;
	tdef = NULL;
	admin_tdef = NULL;
	
	unsigned int a = time(NULL);
	a = rand_r(&a) + (long)this;
	lastSN = rand_r(&a);

	completed = 0;
	badkey = 0;
	badname = 0;
	autoping = 0;
	errstr = NULL;

	timeout = 5000;
	realtmo = 0;

	iservice = NULL;
	executor = NULL;
	netfd = -1;
	lastAct = 0;
	pingReq = NULL;

	ownerPool = NULL;
	ownerId = 0;
	dc = DataConnector::getInstance();

}


// copy most members, zero connection state
// copy address if use generic address
NCServer::NCServer(const NCServer &that) :
	addr(that.addr),
	keyinfo(that.keyinfo)
{
#define _MCOPY(x) this->x = that.x
#define _MZERO(x) this->x = 0
#define _MNEG(x) this->x = -1
#define _MDUP(x)	this->x = that.x ? STRDUP(that.x) : NULL

	_MDUP(tablename);
	_MDUP(appname);
	_MCOPY(keytype);
	_MCOPY(autoUpdateTable);
	_MCOPY(autoReconnect);

	_MCOPY(completed);
	_MCOPY(badkey);
	_MCOPY(badname);
	_MCOPY(autoping);
	// errstr always compile time const string
	_MCOPY(errstr);

	_MCOPY(tdef);
	_MCOPY(admin_tdef);
	if(tdef) tdef->INC();
	if(admin_tdef) admin_tdef->INC();

	lastAct = time(NULL);
	_MCOPY(timeout);
	_MZERO(realtmo); // this is sockopt set to real netfd
	_MCOPY(iservice);
	_MCOPY(executor);
	_MNEG(netfd);	// real socket fd, cleared
	_MZERO(pingReq); // cached ping request object

	unsigned int a = lastAct;
	a = rand_r(&a) + (long)this;
	lastSN = rand_r(&a);


	_MZERO(ownerPool); // serverpool linkage
	_MZERO(ownerId); // serverpool linkage

#undef _MCOPY
#undef _MZERO
#undef _MNEG
#undef _MDUP
}

NCServer::~NCServer(void) {
	DELETE(pingReq);
	Close();
	FREE_IF(tablename);
	FREE_IF(appname);
	DEC_DELETE(tdef);
	DEC_DELETE(admin_tdef);
	DataConnector::getInstance()->SendData();		// 防止10s 间隔时间未到，程序先结束了少报一次
}

DataConnector*  NCServer::dc = NULL;//DataConnector::getInstance();

void NCServer::CloneTabDef(const NCServer& source)
{
	DEC_DELETE(tdef);
	tdef = source.tdef;
	tdef->INC();
	if(tablename != NULL)
		FREE(tablename);
	tablename = STRDUP(source.tablename);
	keytype = source.keytype;
	completed = 1;
}

int NCServer::SetAddress(const char *h, const char *p) {
	if(h==NULL) {
		errstr = "No Host Specified";
		return -EC_BAD_HOST_STRING;
	}

	char *oldhost = NULL;
	if(addr.Name() != NULL) {
		// dup string put on stack
		oldhost = strdupa(addr.Name());
	}

	const char *err = addr.SetAddress(h, p);
	if(err) {
		this->errstr = err;
		return -EC_BAD_HOST_STRING;
	}

	// un-changed
	if(oldhost!=NULL && !strcmp(addr.Name(), oldhost)) 
		return 0;

	Close();

    //set network model
	executor = NULL;
    if(!_network_mode&&iservice && iservice->MatchListeningPorts(addr.Name(), NULL)) {
        executor = iservice->QueryTaskExecutor();
        CTableDefinition *t1 = iservice->QueryTableDefinition();
        if(t1 != this->tdef) {
            DEC_DELETE(this->tdef);
            this->tdef = t1;
        }
        CTableDefinition *t2 = iservice->QueryAdminTableDefinition();
        if(t2 != this->tdef) {
            DEC_DELETE(this->admin_tdef);
            this->admin_tdef = t2;
        }
    }

	return 0;
}

int NCServer::IntKey(void) {
	if(keytype != DField::None && keytype != DField::Signed)
	    return -EC_BAD_KEY_TYPE;
	keytype = DField::Signed;
	if(tablename != NULL) completed = 1;
	return 0;
}

int NCServer::StringKey(void) {
	if(keytype != DField::None && keytype != DField::String)
	    return -EC_BAD_KEY_TYPE;
	keytype = DField::String;
	if(tablename != NULL) completed = 1;
	return 0;
}

int NCServer::AddKey(const char* name, uint8_t type)
{
	int ret = keyinfo.AddKey(name, type);
	// TODO verify tabledef
	if(ret==0) {
		if(tablename != NULL) completed = 1;
		if(keytype == DField::None)
			keytype = type;
	}
	return 0;
}

int NCServer::SetTableName(const char *name) {
	if(name==NULL) return -EC_BAD_TABLE_NAME;

	if(tablename)
		return mystrcmp(name, tablename, 256)==0 ? 0 : -EC_BAD_TABLE_NAME;

	tablename = STRDUP(name);

    if(&NCServer::CheckInternalService != 0 && !_network_mode) {
        CheckInternalService();
    }

	if(keytype != DField::None) completed = 1;

	return 0;
}

int NCServer::FieldType(const char *name) {
	if(tdef == NULL)
		Ping();

	if(tdef != NULL){
		int id = tdef->FieldId(name);
		if(id >= 0)
			return tdef->FieldType(id);
	}
	
	return DField::None;
}

//fixed, sec --> msec
void NCServer::SetMTimeout(int n) {
	timeout = n<=0 ? 5000 : n;
	UpdateTimeout();
}

void NCServer::UpdateTimeout(void) {
	if(netfd >= 0 && realtmo != timeout) {
		struct timeval tv = { timeout/1000, (timeout%1000)*1000 };
		setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		if(timeout) {
			tv.tv_usec += 100000; // add more 100ms
			if(tv.tv_usec >= 1000000)
			{
				tv.tv_usec -= 1000000;
				tv.tv_sec  +=1;
			}
		}
		setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		realtmo = timeout;
	}
}

void NCServer::Close(void){
	if(netfd>=0) {
		close(netfd);
		netfd = -1;
		realtmo=0;
	}
}

int NCServer::BindTempUnixSocket(void) {
	struct sockaddr_un tempaddr;
	tempaddr.sun_family = AF_UNIX;
	snprintf(tempaddr.sun_path, sizeof(tempaddr.sun_path),
			"@ttcapi-%d-%d-%d", getpid(), netfd, (int)time(NULL));
	socklen_t len = SUN_LEN(&tempaddr);
	tempaddr.sun_path[0] = 0;
	return bind(netfd, (const sockaddr *)&tempaddr, len);
}

int NCServer::Connect(void) {
	if(ownerPool != NULL)
		return -EC_PARALLEL_MODE;
	if(executor != NULL)
		return 0;
	if(netfd >= 0)
		return 0;

	int err = -EC_NOT_INITIALIZED;

	if(addr.SocketFamily() != 0) {
		netfd = addr.CreateSocket();
		if(netfd < 0) {
			log_error("create socket error: %d, %m", errno);
			err = -errno;
		} else if(addr.SocketFamily()==AF_UNIX && IsDgram() && BindTempUnixSocket() < 0) {
			log_error("bind unix socket error: %d, %m", errno);
			err = -errno;
			Close();
		} else {
			UpdateTimeout();
			if(addr.ConnectSocket(netfd)==0) {
				return 0;
			}
			log_error("connect ttc server error: %d,%m", errno);
			err = -errno;
			if(err==-EINPROGRESS) err = -ETIMEDOUT;
			Close();
		}
	}
	return err;
}

int NCServer::Reconnect(void) {
	if(ownerPool != NULL)
		return -EC_PARALLEL_MODE;
	if(executor != NULL)
		return 0;
	if(netfd < 0)
		return -ENOTCONN;

	if(addr.ConnectSocket(netfd)==0) {
		return 0;
	}
	log_error("connect ttc server error: %d,%m", errno);
	int err = -errno;
	if(err==-EINPROGRESS) err = -ETIMEDOUT;
	Close();
	return err;
}

int NCServer::SendPacketStream(CPacket &pk) {
	int err;
#ifndef SIOCOUTQ
	if(1) //if(!IsDgram())
	{
		char tmp[1];
		err = recv(netfd, tmp, sizeof(tmp), MSG_PEEK|MSG_DONTWAIT);
		if(err==0 || (err<0 && errno != EAGAIN && errno != EWOULDBLOCK))
		{
			if(!autoReconnect)
			{
				log_error("ttc server close connection: %d,%m", err);
				return err;
			}
			else
			{
				log_debug("%s", "ttc server close connection, re-connect now.");
				Close();
				err = Connect();
				if(err) return err;
			}
		}
	}
#endif

	while((err = pk.Send(netfd)) == SendResultMoreData)
	{
	    if(errno==EINPROGRESS)
	    {
			log_error("socket[%d] send data timeout: %d,%m", netfd, errno);
	    	// timeout
			if(1) // if(!IsDgram())
				Close();
			return -ETIMEDOUT;
	    }
	}
	if(err != SendResultDone)
	{
		log_error("socket[%d] send data error: %d,%m", netfd, err);
	    err = -errno;
	    if(1) // if(!IsDgram())
		    Close();
	    return err;
	}
	return 0;
}

// always return network error code
// EAGAIN must re-sent
int NCServer::DecodeResultStream(NCResult &tk) 
{
	if(netfd < 0) 
	{
		tk.SetError(-ENOTCONN, "API::recving", "connection is closed");
		return -ENOTCONN;
	}

	int err;
	CSimpleReceiver receiver(netfd);

	uint64_t beginTime = 0;
	//log_debug("wait response time stamp = %lld", (long long)(beginTime = GET_TIMESTAMP()));
	
	while(1)
	{
		errno = 0;
		err = tk.Decode(receiver);

		if(err == DecodeFatalError) 
		{
			log_error("socket[%d] decode data error: %d, %m", netfd, errno);
			
			err = -errno;
			if(err==0)
				err = -ECONNRESET;
#ifdef SIOCOUTQ
			int value = 0;
			ioctl(netfd, SIOCOUTQ, &value);
			// unsent bytes
			if(value > 0) {
				err = -EAGAIN;
				tk.SetError(err, "API::sending", "client send packet error");
				Close();
				return err;
			}
#endif
			Close();
			// EAGAIN never return FatalError, should be DecodeWaitData
			tk.SetError(err, "API::recving", "client recv packet error");
			return err;
		}

		if(err == DecodeDataError)
		{
			log_error("socket[%d] decode data error: %d, %m", netfd, errno);
			
			// packet is valid
			return 0;
		}

		if(err == DecodeDone)
		{
			break;
		}

		if(errno == EINPROGRESS || errno == EAGAIN)
		{
			log_error("socket[%d] decode data error: %d, %m", netfd, errno);
			log_debug("use time %ldms", (long)((GET_TIMESTAMP()-beginTime)/1000));
			
			Close();
			tk.SetError(-ETIMEDOUT, "API::recving", "client recv packet timedout");
			return -ETIMEDOUT;
		}
	}

	//log_debug("decode result use time: %ldms", (long)((GET_TIMESTAMP()-beginTime)/1000));
	
	SaveDefinition(&tk);
	if(autoping) {
		time(&lastAct);
		err = tk.versionInfo.KeepAliveTimeout();
		if(err<15) err = 15;
		lastAct += err - 1;
	}
	return 0;
}

int NCServer::SendPacketDgram(CSocketAddress *peer, CPacket &pk)
{
	int err = 0;

	while ((err = pk.SendTo(netfd, peer)) == SendResultMoreData)
	{
		if (errno == EINPROGRESS)
		{
			log_error("socket[%d] send data timeout: %d,%m", netfd, errno);
			// timeout
			return -ETIMEDOUT;
		}
	}

	if (err != SendResultDone)
	{
		log_error("socket[%d] send data error: %d,%m", netfd, err);
		err = -errno;
		return err;
	}
	return 0;
}

static inline char *RecvFromFixedSize(int fd, int len, int &err, struct sockaddr *peeraddr, socklen_t *addrlen)
{
	int blen = len <= 0 ? 1 : len;
	char *buf = (char *)MALLOC(blen);
	blen = recvfrom(fd, buf, blen, 0, peeraddr, addrlen);
	if(blen < 0) {
		err = errno;
		free(buf);
		buf = NULL;
	} else if(blen != len) {
		err = EIO;
		free(buf);
		buf = NULL;
	} else {
		err = 0;
	}

	return buf;
}

static char *RecvPacketPeek(int fd, int &len, int &err)
{
	// MSG_PEEK + MSG_TRUNC get next packet size
	char dummy[1];
	len = recv(fd, dummy, 1, MSG_PEEK|MSG_TRUNC);
	if(len < 0) {
		err = errno;
		return NULL;
	}

	return RecvFromFixedSize(fd, len, err, NULL, NULL);
}

static char *RecvPacketPeekPeer(int fd, CSocketAddress *peer, int &len, int &err)
{
	struct sockaddr peeraddr;
	socklen_t sock_len;

	for (int n=0; n<10; n++) {
		char dummy[1];

		sock_len = sizeof(peeraddr);
		len = recvfrom(fd, dummy, 1, MSG_PEEK|MSG_TRUNC, &peeraddr, &sock_len);
		if(len < 0) {
			err = errno;
			return NULL;
		}

		if(peer->Equal(&peeraddr, sock_len, SOCK_DGRAM))
			break;
	}

	return RecvFromFixedSize(fd, len, err, &peeraddr, &sock_len);
}

static char *RecvPacketPoll(int fd, int msec, int &len, int &err)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	int n;
	
	while( (n = poll(&pfd, 1, msec)) < 0 && errno == EINTR)
		/* LOOP IF EINTR */;

	if(n == 0) {
		err = EAGAIN;
		return NULL;
	}
	if(n < 0) {
		err = errno;
		return NULL;
	}

	len = 0;
	ioctl(fd, SIOCINQ, &len);

	return RecvFromFixedSize(fd, len, err, NULL, NULL);
}

// always return network error code
// EAGAIN must re-sent
int NCServer::DecodeResultDgram(CSocketAddress *peer, NCResult &tk) 
{
	if(netfd < 0) 
	{
		tk.SetError(-ENOTCONN, "API::recving", "connection is closed");
		return -ENOTCONN;
	}

	int saved_errno; // saved errno
	int len; // recv len
	char *buf; // packet buffer
	uint64_t beginTime = 0;

	log_debug("wait response time stamp = %lld", (long long)(beginTime = GET_TIMESTAMP()));
	
	if(addr.SocketFamily() == AF_UNIX) {
		if(peer == NULL) {
			buf = RecvPacketPoll(netfd, timeout,  len, saved_errno);
		} else {
			if(peer->ConnectSocket(netfd) < 0) {
				saved_errno = errno;
				log_error("connect ttc server error: %d,%m", saved_errno);
				if(saved_errno == EAGAIN) {
					// unix socket return EAGAIN if listen queue overflow
					saved_errno = EC_SERVER_BUSY;
				}
				tk.SetError(-saved_errno, "API::connecting", "client api connect server error");
				return -saved_errno;
			}

			buf = RecvPacketPoll(netfd, timeout, len, saved_errno);

			// Un-connect
			struct sockaddr addr;
			addr.sa_family = AF_UNSPEC;
			connect(netfd, &addr, sizeof(addr));
		}
	} else {
		if(peer == NULL) {
			buf = RecvPacketPeek(netfd, len, saved_errno);
		} else {
			buf = RecvPacketPeekPeer(netfd, peer, len, saved_errno);
		}
	}
	log_debug("receive result use time: %ldms", (long)((GET_TIMESTAMP()-beginTime)/1000));

	if(buf == NULL)
	{
		errno = saved_errno;
		log_error("socket[%d] recv udp data error: %d,%m", netfd, saved_errno);

		if(saved_errno == EAGAIN) {
			// change errcode to TIMEDOUT;
			saved_errno = ETIMEDOUT;
			tk.SetError(-ETIMEDOUT, "API::recving", "client recv packet timedout");
		} else {
			if(saved_errno==0)
				saved_errno = ECONNRESET;
			tk.SetError(-saved_errno, "API::recving", "connection reset by peer");
		}
		return -saved_errno;
	}

	int err = tk.Decode(buf, len, 1 /*eat buf*/);

	switch(err) {
	case DecodeFatalError:
		log_error("socket[%d] decode udp data error: invalid packet content", netfd);
		tk.SetError(-ECONNRESET, "API::recving", "invalid packet content");
		FREE_IF(buf); // buf not eaten
		return -ECONNRESET;

	case DecodeWaitData:
	default:
		// UNREACHABLE, DecodePacket never return these code
		log_error("socket[%d] decode udp data error: %d, %m", netfd, errno);
		tk.SetError(-ETIMEDOUT, "API::recving", "client recv packet timedout");
		return -ETIMEDOUT;

	case DecodeDataError:
		// packet maybe valid
		break;

	case DecodeDone:
		SaveDefinition(&tk);
		break;

	}

	return 0;
}

//Get udpport form udppool
NCUdpPort *NCServer::GetGlobalPort()
{
    NCUdpPort *udpport = NCUdpPort::Get(addr.SocketFamily());

    if (udpport)
    {
	    netfd = udpport->fd;
	    lastSN = udpport->sn;
	    realtmo = udpport->timeout; 
	    UpdateTimeout();
    }

    return udpport;
}

//Put the udpport to the udppool
void NCServer::PutGlobalPort(NCUdpPort *udpport)
{
	if (udpport)
	{
		if (netfd > 0)
		{
			udpport->sn = lastSN;
			udpport->timeout = realtmo;
			netfd = -1;
			udpport->Put();
		}
		else
		{
			//if fd error, delete this udpport object
			udpport->Eat();
		}
	}
}

void NCServer::SaveDefinition(NCResult *tk)
{
	if(strcmp("*", tablename)!=0 && tk->ResultCode()==-EC_TABLE_MISMATCH) {
		badname = 1;
		errstr = "Table Name Mismatch";
		return;
	}
	if(strcmp("*", tablename)!=0 && tk->ResultCode()==-EC_BAD_KEY_TYPE) {
		badkey = 1;
		errstr = "Key Type Mismatch";
		return;
	}
	
	CTableDefinition *t = tk->RemoteTableDefinition();
	if(t == NULL) return;

	if(t->IsAdminTable()){
		if(admin_tdef)
		{
			if(admin_tdef->IsSameTable(t)) return;
			if(!autoUpdateTable){
				badname = 1;
				errstr = "Table Mismatch";
				tk->SetError(-EC_TABLE_MISMATCH, "API::executed", "AdminTable Mismatch");
				return;
			}
			DEC_DELETE(admin_tdef);
		}
		admin_tdef = t;
		admin_tdef->INC();
	}
	else{
		if(tdef)
		{
			if(tdef->IsSameTable(t)) return;
			if(!autoUpdateTable){
				badname = 1;
				errstr = "Table Mismatch";
				tk->SetError(-EC_TABLE_MISMATCH, "API::executed", "Table Mismatch");
				return;
			}
			DEC_DELETE(tdef);
		}
		tdef = t;
		tdef->INC();
		
		FREE(tablename);
		tablename = STRDUP(tdef->TableName());
		keytype = tdef->KeyType();

	    //bugfix， by ada
	    if(keytype == DField::Unsigned)
	        keytype = DField::Signed;

	    if(keytype == DField::Binary)
	        keytype = DField::String;
	}
}

void NCServer::TryPing(void) {
	if(autoping && netfd >= 0) {
		time_t now;
		time(&now);
		if(now >= lastAct)
			Ping();
	}
}

int NCServer::Ping(void) {
	if(ownerPool != NULL)
		return -EC_PARALLEL_MODE;
	if(executor != NULL)
		return 0;
		NCRequest r(this, DRequest::Nop);
	NCResult *t = r.ExecuteNetwork();
	int ret = t->ResultCode();
	delete t;
	return ret;
}

NCResult *NCServer::DecodeBuffer(const char *buf, int len)
{
	NCResult *res = new NCResult(tdef);

	switch(len <= (int)sizeof(CPacketHeader) ? DecodeFatalError : res->Decode(buf, len))
	{
		default:
			res->SetError(-EINVAL, "API::decoding", "invalid packet content");
			break;
		case DecodeDataError:
		case DecodeDone:
			break;
	}
	return res;
}

int NCServer::CheckPacketSize(const char *buf, int len)
{
	return NCResult::CheckPacketSize(buf, len);
}

/*date:2014/06/04, author:xuxinxin*/
int NCServer::SetAccessKey(const char *token)
{
	int ret = 0;
	std::string str;
	if(token == NULL)
		return -EC_BAD_ACCESS_KEY;
	else
		str = token;
	if(str.length() != ACCESS_KEY_LEN)
	{
		log_error("Invalid accessKey!");
		accessToken = "";
		return -EC_BAD_ACCESS_KEY;
	}
	else
		accessToken = str;

	/* 截取  AccessKey中前32位以后的bid， 以便上报至数据采集 dtchttpd */
	ret = dc->SetBussinessId(accessToken);
	return 0;
}

DataConnector* DataConnector::pDataConnector = NULL;

void *DataConnector::Process(void)
{
	pid = _gettid_();
	int err = 0;
	int innerRet = 0;
    while(1)
    {
    	log_info("upload data thread enter! pid:%d", pid);
    	uint64_t time_before = 0;
    	uint64_t time_after = 0;
    	timeval tv;
    	tv.tv_sec = 10 - (time_after - time_before);
		
		if (tv.tv_sec < 0)
			tv.tv_sec = 0;
			
    	tv.tv_usec = 0;
    	// 以10s为周期，上报数据
    	err = select(0, NULL, NULL, NULL, &tv);
    	time_before = GET_TIMESTAMP();
    	if(0 == err)
    	{
    		innerRet = DataConnector::getInstance()->SendData();
    		time_after = GET_TIMESTAMP();
    	}
    }
    return NULL;
}

/* date:2014/06/09, author:xuxinxin */
DataConnector::DataConnector():CThread("UploadThread", CThread::ThreadTypeAsync)
{
}

DataConnector::~DataConnector()
{
}

int DataConnector::SendData()
{
	pid = _gettid_();
	int ret = 0;
	std::map<uint32_t, businessStatistics> mapData;
	GetReportInfo(mapData);
	int flag = 0;
	for(std::map<uint32_t, businessStatistics>::iterator itrflag = mapData.begin(); itrflag != mapData.end(); ++itrflag)
	{
		flag = itrflag->second.TotalTime +  itrflag->second.TotalRequests;
	}
	if(flag == 0)
	{
		return 0;
	}
	int HttpServiceTimeOut = 2;
	std::string strServiceUrl = "http://report.dtc.jd.com:9090";
	std::string strRsp = "";
	CurlHttp curlHttp;
	Json::FastWriter writer;
	Json::Value statics;

	for(std::map<uint32_t, businessStatistics>::iterator itr = mapData.begin(); itr != mapData.end(); ++itr)
	{
		log_info("bid:%d, TotalTime:%lu, TotalRequests:%u", itr->first, itr->second.TotalTime, itr->second.TotalRequests);
		uint32_t bid = 0;
		bid = itr->first;

		uint64_t totalTime = itr->second.TotalTime;
		uint32_t reqNum = itr->second.TotalRequests;
		Json::Value content;
		Json::Value	inner;
		Json::Value inArr;

		Json::Value middle;
		if(0 != totalTime && 0 != reqNum)
		{
			inner["curve"] = "5";
			inner["etype"] = "1";
			inner["data1"] = static_cast<Json::UInt64>(totalTime);
			inner["data2"] = reqNum;
			inner["extra"] = "";
			inner["cmd"] = "0";
			inArr.append(inner);

			middle["bid"] = bid;
			middle["content"] =inArr;

			statics.append(middle);
		}
		else
		{
			log_error("empty data in bid:[%d]!", bid);
		}

	}
	Json::Value body;
	body["cmd"] = 3;
	body["statics"]=statics;
	BuffV buf;
	std::string strBody = writer.write(body);
	log_info("HttpBody = [%s]", strBody.c_str());
	curlHttp.SetHttpParams("%s", strBody.c_str());
	curlHttp.SetTimeout(HttpServiceTimeOut);
	ret = curlHttp.HttpRequest(strServiceUrl, &buf, false);

	strRsp = buf.Ptr();
	log_info("ret:%d, strRsp:%s", ret, strRsp.c_str());

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(strRsp.c_str(), root))
	{
		log_error("parse Json failed, strRsp:%s", strRsp.c_str());
	}

	std::string strRetCode = root["retCode"].asString();
	log_info("strRetCode:%s", strRetCode.c_str());

	if(strRetCode == "1")
	{
		log_info("call dtchttpd success!");
	}
	else
	{
		log_error("call dtchttpd failed! strRetCode:%s", strRetCode.c_str());
	}
	return 0;
}

int DataConnector::SetReportInfo(std::string str, uint64_t t)
{
	uint32_t bid = 0;
	std::string stemp = str.substr(0,8);
	sscanf(stemp.c_str(), "%u", &bid);

	if(mapBi.find(bid) != mapBi.end())
	{
		do
		{
			CScopedLock lock(_lock);
			mapBi[bid].TotalTime += t;
			mapBi[bid].TotalRequests += 1;
		}while(0);
	}
	return 0;
}

void DataConnector::GetReportInfo(std::map<uint32_t, businessStatistics> &map)
{
	CScopedLock lock(_lock);
	for(std::map<uint32_t, businessStatistics>::iterator it = mapBi.begin(); it != mapBi.end(); ++it)
	{
		struct businessStatistics bs;
		map[it->first] = it->second;
		/*  mapBi 取出来之后，不管后面是否上报成功，该bid对应的数据都丢弃，重置为0  */
		mapBi[it->first] = bs;
	}
}

int DataConnector::SetBussinessId(std::string str)
{
	CScopedLock lock(_lock);
	if(mapBi.size() == 0)
	{
		InitializeThread();
		RunningThread();
	}
	std::string stemp = str.substr(0, 8);
	uint32_t bid = 0;
	sscanf(stemp.c_str(), "%u", &bid);

	struct businessStatistics bs;
	mapBi.insert(std::make_pair(bid, bs));
	return 0;
}
