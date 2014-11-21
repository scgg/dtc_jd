#include "ttcint.h"
#include <stdio.h>
#include "protocol.h"
#include "cache_error.h"
#include "receiver.h"
#include <time.h>
#include <stdlib.h>

const uint32_t ACCESS_KEY_LEN = 40;


NCServer::NCServer(void)
{

	keytype = DField::None;

	tablename = NULL;
	appname = NULL;

	autoUpdateTable = 1;
	autoReconnect = 1;
	tdef = NULL;
	admin_tdef = NULL;



	completed = 0;
	badkey = 0;
	badname = 0;
	autoping = 0;
	errstr = NULL;

	timeout = 5000;
	realtmo = 0;


	executor = NULL;
	netfd = -1;


	ownerPool = NULL;


	unsigned int a = time(NULL);
	srand(a);
	a = rand() + (long)this;
	lastSN = rand();


	
}


NCServer::~NCServer(void) {


	Close();
//	delete tablename;
//	FREE_IF(tablename);
//	FREE_IF(appname);
	DEC_DELETE(tdef);
	DEC_DELETE(admin_tdef);


}


//key 类型
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

//设置表名
int NCServer::SetTableName(const char *name) {
	if(name==NULL) return -EC_BAD_TABLE_NAME;

	if(tablename)
		return mystrcmp(name, tablename, 256)==0 ? 0 : -EC_BAD_TABLE_NAME;
	
	if (tablename==NULL)
	{
		tablename=new char(strlen(name));
	}
	memcpy(tablename,name,strlen(name)+1);
	//tablename = STRDUP(name);




	if(keytype != DField::None) completed = 1;

	return 0;
}



int NCServer::SetAddress(const char *h, const char *p) {
	if(h==NULL) {
		errstr = "No Host Specified";
		return -EC_BAD_HOST_STRING;
	}

	char *oldhost =NULL; 
	if(addr.Name() != NULL) {
		oldhost=new char(strlen(addr.Name()));
		memcpy(oldhost,addr.Name(),strlen(addr.Name())+1);
	}

	const char *err = addr.SetAddress(h, p);
	if(err) {
		this->errstr = err;
		return -EC_BAD_HOST_STRING;
	}

	// un-changed
	if(oldhost!=NULL && !strcmp(addr.Name(), oldhost)) 
		return 0;

	//关闭已连接套接字
	Close();

	

	return 0;
}


void NCServer::Close(void){
	if(netfd>=0) {
		closesocket(netfd);
		netfd = -1;
		realtmo=0;
	}
}

//fixed, sec --> msec
void NCServer::SetMTimeout(int n) {
	timeout=n<=0 ? 5000 : n;
	UpdateTimeout();
}


void NCServer::UpdateTimeout(void) {
	if(netfd >= 0 && realtmo != timeout) {
		struct timeval tv= { timeout/1000, (timeout%1000)*1000 };
		setsockopt(netfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
		if(timeout) {
			tv.tv_usec += 100000; // add more 100ms
			if(tv.tv_usec >= 1000000)
			{
				tv.tv_usec -= 1000000;
				tv.tv_sec  +=1;
			}
		}
		setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
		realtmo = timeout;
	}
}



/*date:2014/06/04, author:xuxinxin*/
int NCServer::SetAccessKey(const char *token)
{
	int ret = 0;
	string str;
	if(token == NULL)
		return -EC_BAD_ACCESS_KEY;
	else
		str = token;
	if(str.length() != ACCESS_KEY_LEN)
	{
//		log_error("Invalid accessKey!");
		accessToken = "";
		return -EC_BAD_ACCESS_KEY;
	}
	else
		accessToken = str;

	/* 截取  AccessKey中前32位以后的bid， 以便上报至数据采集 dtchttpd */
//	ret = dc->SetBussinessId(accessToken);
	return 0;
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
			printf("create socket error: %d, %m", errno);
			err = -errno;
		} else if(addr.SocketFamily()==AF_UNIX && IsDgram()) {
			printf("bind unix socket error: %d, %m", errno);
			err = -errno;
			Close();
		} else {

 

			UpdateTimeout();
		
			if(addr.ConnectSocket(netfd)==0) {
				return 0;
			}
			printf("connect ttc server error: %d,%m", errno);
			err = -errno;
			if(err==-EINPROGRESS) err = -ETIMEDOUT;
			Close();
		}
	}
	return err;
}



int NCServer::SendPacketStream(CPacket &pk) {
	int err=0;

	while((err = pk.Send(netfd)) == SendResultMoreData)
	{
		if(errno==EINPROGRESS)
		{
			printf("socket[%d] send data timeout: %d,%m", netfd, errno);
			// timeout
			if(1) // if(!IsDgram())
				Close();
			return -ETIMEDOUT;
		}
	}
	if(err != SendResultDone)
	{
		printf("socket[%d] send data error: %d,%m", netfd, err);
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
		printf("API::recving", "connection is closed\n");
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
			printf("socket[%d] decode data error: %d, %m\n", netfd, errno);

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
			printf("API::recving,client recv packet error %d\n",err);
			return err;
		}

		if(err == DecodeDataError)
		{
			printf("socket[%d] decode data error: %d, %m\n", netfd, errno);

			// packet is valid
			return 0;
		}

		if(err == DecodeDone)
		{
			break;
		}

		if(errno == EINPROGRESS || errno == EAGAIN)
		{
			printf("socket[%d] decode data error: %d, %m\n", netfd, errno);
			Close();
			return -ETIMEDOUT;
		}
	}

	//log_debug("decode result use time: %ldms", (long)((GET_TIMESTAMP()-beginTime)/1000));

	SaveDefinition(&tk);
	if(autoping) {
		err = tk.versionInfo.KeepAliveTimeout();
		if(err<15) err = 15;
	}
	return 0;
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
				printf("API::executed,Table Mismatch\n");
				return;
			}
			DEC_DELETE(tdef);
		}
		tdef = t;
		tdef->INC();

		memset(tablename,0,strlen(tablename)+1);
		//FREE(tablename);
		memcpy(tablename,tdef->TableName(),strlen(tdef->TableName())+1);
		keytype = tdef->KeyType();

		//bugfix， by ada
		if(keytype == DField::Unsigned)
			keytype = DField::Signed;

		if(keytype == DField::Binary)
			keytype = DField::String;
	}
}
