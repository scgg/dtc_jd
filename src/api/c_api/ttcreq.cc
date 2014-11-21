#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <new>

#include "ttcint.h"

// NCRequest's methods always assign and return
// cache the logical error code
#define return_err(x)	return err = (x)
#define return_err_res(x,y,z)   return new NCResult(x,y,z)

NCRequest::NCRequest(NCServer *s, int op)
{
    compressFlag = 0;
	if( op != DRequest::Monitor && (s && s->IsCompleted()==0) )//MonitorRequest need not table check
		s = NULL;
	server = s;
	if(server) server->INC();
	tdef = NULL;
	tablename = NULL;
	keytype = 0;

	switch(op) {
	case DRequest::Nop:
	case DRequest::Get:
	case DRequest::Purge:
	case DRequest::Flush:
	case DRequest::Insert:
	case DRequest::Update:
	case DRequest::Delete:
	case DRequest::Replace:
		if(server){
			tdef = server->tdef;
			tablename = server->tablename;
			keytype = server->keytype;
		}
		break;
	//跨IDC分布支持
	case DRequest::SvrAdmin:
	case DRequest::Invalidate:
		if(server){
			tdef = server->admin_tdef;
			tablename =(char *) "@HOT_BACKUP";
			keytype = DField::Unsigned;
		}
		break;
	case DRequest::Monitor:
		break;
	default:
		op = DRequest::ResultCode;
	}
	cmd = op;
	err = 0;
	haskey = 0;
	flags = 0;
	key.u64 = 0;
	key.bin.ptr = NULL;
	key.bin.len = 0;

	limitStart = 0;
	limitCount = 0;
	adminCode = 0;
	hotBackupID = 0;
	MasterHBTimestamp = 0;
	SlaveHBTimestamp = 0;
	if(server)
		kvl.keyinfo = &server->keyinfo;
    gzip = NULL;
}

NCRequest::~NCRequest(void){
	UnsetKeyValue();
	UnsetKey();
	if(server) DEC_DELETE(server);
    if(gzip) DELETE(gzip);
}

// AttachServer() error is transient, don't cache it
int NCRequest::AttachServer(NCServer *s)
{
	// NO-OP
	if(s==server)
		return 0;

	switch(cmd) {
	case DRequest::Nop:
	case DRequest::Get:
	case DRequest::Purge:
	case DRequest::Flush:
	case DRequest::Insert:
	case DRequest::Update:
	case DRequest::Delete:
	case DRequest::Replace:
		break;
	default:
		// don't allow change admin target server
		return -EPERM;
	}

	/* new server must be initialized */
	if(s==NULL || s->IsCompleted()==0)
		return -EC_NOT_INITIALIZED;

	/* precheck same table */
	if(server) {
		if(keytype != s->keytype)
			return -EC_BAD_KEY_TYPE;
		if(!server->keyinfo.Equal(s->keyinfo))
			return -EC_BAD_KEY_TYPE;
		if(strcmp(tablename, s->tablename) != 0)
			return -EC_BAD_TABLE_NAME;
		if(tdef==NULL) {
			// no current tabledef
		} else if(tdef == server->tdef) {
			// same tabledef
		} else if(tdef->IsSameTable(server->tdef)) {
			// hash equal
		} else {
			// force re-resolve fieldnames
			ui.Unresolve();
			ci.Unresolve();
			fs.Unresolve();
		}

		// release old server
		DEC_DELETE(server);
	}

	// switch to new server
	server = s;
	server->INC();
	tdef = server->tdef;
	tablename = server->tablename;
	keytype = server->keytype;
	kvl.keyinfo = &server->keyinfo;
	return 0;
}

int NCRequest::Need(const char *n, int vid)
{
	if(server==NULL)
		return_err(-EC_NOT_INITIALIZED);
	if(cmd!=DRequest::Get && cmd!=DRequest::SvrAdmin)
		return_err(-EC_BAD_OPERATOR);
	int ret = fs.AddField(n, vid);
	if(ret) err = ret;
	return ret;
}

int NCRequest::AddCondition(const char *n, uint8_t op, uint8_t t, const CValue &v)
{
	if(server==NULL)
		return_err(-EC_NOT_INITIALIZED);
	if(cmd==DRequest::Insert || cmd==DRequest::Replace)
		return_err(-EC_BAD_OPERATOR);
	switch(t) {
	case DField::Signed:
	case DField::Unsigned:
	    if(op >= DField::TotalComparison)
		    return_err(-EC_BAD_OPERATOR);
	    break;
	case DField::String:
	case DField::Binary:
	    if(op != DField::EQ && op != DField::NE)
		    return_err(-EC_BAD_OPERATOR);
	    break;
	default:
	    return_err(-EC_BAD_FIELD_TYPE);
	}
	int ret = ci.AddValue(n, op, t, v);
	if(ret) err = ret;
	return ret;
}

int NCRequest::AddOperation(const char *n, uint8_t op, uint8_t t, const CValue &v)
{
	if(server==NULL) return_err(-EC_NOT_INITIALIZED);
	switch(cmd)
	{
		case DRequest::Insert:
		case DRequest::Replace:
		case DRequest::SvrAdmin:
			if(op != DField::Set)
				return_err(-EC_BAD_OPERATOR);
			break;
		case DRequest::Update:
			break;
		default:
			return_err(-EC_BAD_OPERATOR);
			break;
	}

	switch(t) {
	case DField::Signed:
	case DField::Unsigned:
	    if(op >= DField::TotalOperation)
		    return_err(-EC_BAD_OPERATOR);
	    break;

	case DField::String:
	case DField::Binary:
	    if(op == DField::Add||op == DField::OR) return_err(-EC_BAD_OPERATOR);
	    break;

	case DField::Float:
	    if(op == DField::SetBits || op == DField::OR)
		    return_err(-EC_BAD_OPERATOR);
	    break;
	default:
	    return_err(-EC_BAD_FIELD_TYPE);
	}
	int ret = ui.AddValue(n, op, t, v);
	if(ret) err = ret;
	return ret;
}

int NCRequest::initCompress()
{
    int iret = 0;
    if (server==NULL)
        return -EC_NOT_INITIALIZED;
    if (tdef==NULL)
    {
        iret = server->Ping();//ping and get tabledef
        tdef = server->tdef;
    }
    if (iret)
        return iret;
    if (gzip==NULL)
        NEW(CCompress,gzip);
    if (gzip==NULL)
        return -ENOMEM;
    if (tdef==NULL)
        return -EC_CHECKSUM_MISMATCH;
    if (server->GetCompressLevel())
    {
        gzip->SetCompressLevel(server->GetCompressLevel());
    }
    return gzip->SetBufferLen(tdef->MaxFieldSize());
}

int NCRequest::CompressSet(const char *n,const char * v,int len)
{
    int iret = 0;
    if (len < 0)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldlen is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (n == NULL)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldname is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (v == NULL)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldvalue is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (gzip==NULL)
    {
        iret = initCompress();
        if (iret) return iret;
    }
    if (tdef->CompressFieldId() <= 0)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:TTC must add a field for compress(FieldType=2,FieldSize=8,DefaultValue=compressflag)");
        return -EC_COMPRESS_ERROR;
    }

    if (tdef->FieldType(tdef->FieldId(n)) != DField::Binary)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:compress just support binary field");
        return - EC_BAD_VALUE_TYPE;
    }

    iret = gzip->Compress(v,len);
    if (iret)
    {
        if (iret==-111111)
             snprintf(_errmsg, sizeof(_errmsg), "compress error:compress buff is null,sth sucks happend");
        else
             snprintf(_errmsg, sizeof(_errmsg), "compress error:zlib return code is %d.",iret);
             return -EC_COMPRESS_ERROR ;
    }
    iret = setCompressFlag(n);
    if (iret) return iret;

    return AddOperation(n,DField::Set,DField::String,CValue::Make(gzip->GetBuf(),gzip->GetLen()));
}

int NCRequest::CompressSetForce(const char *n,const char * v,int len)
{
    int iret = 0;
    if (len < 0)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldlen is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (n == NULL)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldname is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (v == NULL)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:fieldvalue is invalid");
        return -EC_COMPRESS_ERROR;
    }

    if (gzip==NULL)
    {
        iret = initCompress();
        if (iret) return iret;
    }

    if (tdef->FieldType(tdef->FieldId(n)) != DField::Binary)
    {
        snprintf(_errmsg, sizeof(_errmsg), "compress error:compress just support binary field");
        return - EC_BAD_VALUE_TYPE;
    }

    iret = gzip->Compress(v,len);
    if (iret)
    {
        if (iret==-111111)
             snprintf(_errmsg, sizeof(_errmsg), "compress error:compress buff is null,sth sucks happend");
        else
             snprintf(_errmsg, sizeof(_errmsg), "compress error:zlib return code is %d.",iret);
             return -EC_COMPRESS_ERROR ;
    }
    if (iret) return iret;

    return AddOperation(n,DField::Set,DField::String,CValue::Make(gzip->GetBuf(),gzip->GetLen()));
}

int NCRequest::AddValue(const char *n, uint8_t t, const CValue &v)
{
	if(server==NULL) return_err(-EC_NOT_INITIALIZED);
	if(cmd!=DRequest::Insert && cmd!=DRequest::Replace) return_err(-EC_BAD_COMMAND);
	int ret = ui.AddValue(n, DField::Set, t, v);
	if(ret) err = ret;
	return ret;
}

int NCRequest::UnsetKey(void) {
	if(haskey) {
		if(server && server->keytype == DField::String)
		{

			DELETE_ARRAY(key.bin.ptr);
			key.Set(NULL, 0);
		}
		haskey = 0;
	}
	return 0;
}

int NCRequest::UnsetKeyValue(void) {
	kvl.Unset();
	flags &= ~DRequest::Flag::MultiKeyValue;
	return 0;
}

int NCRequest::SetKey(int64_t k) {
	if(server==NULL)
		return_err(-EC_NOT_INITIALIZED);
	if(server->keytype != DField::Signed)
		return_err(-EC_BAD_KEY_TYPE);
	key = k;
	haskey = 1;
	UnsetKeyValue();
	return 0;
}

int NCRequest::SetKey(const char *name, int l) {
	if(server==NULL)
		return_err(-EC_NOT_INITIALIZED);
	if(server->keytype != DField::String)
		return_err(-EC_BAD_KEY_TYPE);
	char *a = new char[l];
	memcpy(a, name, l);
	DELETE_ARRAY(key.bin.ptr);
	haskey = 1;
	key.Set(a, l);
	UnsetKeyValue();
	return 0;
}

int NCRequest::AddKeyValue(const char* name, const CValue &v, uint8_t type)
{
	if(server==NULL)
		return_err(-EC_NOT_INITIALIZED);
	if(server->AllowBatchKey()==0)
		return_err(-EC_NOT_INITIALIZED);

	int ret = kvl.AddValue(name, v, type);
	if(ret < 0)
		return err = ret;

	flags |= DRequest::Flag::MultiKeyValue;
	UnsetKey();
	return 0;
}

int NCRequest::SetTabDef(void)
{
	int ret = 0;
	if(server){
		switch(cmd) {
			case DRequest::Nop:
			case DRequest::Get:
			case DRequest::Purge:
			case DRequest::Flush:
			case DRequest::Insert:
			case DRequest::Update:
			case DRequest::Delete:
			case DRequest::Replace:
				ret = tdef != server->tdef;
				tdef = server->tdef;
				tablename = server->tablename;
				keytype = server->keytype;
				break;
			//跨IDC分布支持
			case DRequest::SvrAdmin:
			case DRequest::Invalidate:
				ret = tdef != server->admin_tdef;
				tdef = server->admin_tdef;
				tablename = (char *)"@HOT_BACKUP";
				keytype = DField::Unsigned;
				break;
			default:
				break;
		}
	}
	return ret;
}

int NCRequest::Encode(const CValue *kptr, CPacket *pkt)
{
	int err;
	int force = SetTabDef();

	if(tdef &&(
				(err = ui.Resolve(tdef, force)) ||
				(err = ci.Resolve(tdef, force)) ||
				(err = fs.Resolve(tdef, force))
			  ))
	{
		return err;
	}

	if((err = pkt->EncodeRequest(*this, kptr)) != 0) {
		return err;
	}
	return 0;
}

NCResult *NCRequest::ExecuteStream(const CValue *kptr)
{
	int resend = 1;
	int nrecv = 0;
	int nsent = 0;

	while(1)
	{
		int err;

		if(resend)
		{
			CPacket pk;

			if((err=Encode(kptr, &pk)) < 0)
			{
				return_err_res(err, "API::encoding", "client api encode error");
			}

			if((err = server->Connect()) != 0)
			{
				if(err==-EAGAIN)
				{
					// unix socket return EAGAIN if listen queue overflow
					err = -EC_SERVER_BUSY;
				}
				return_err_res(err, "API::connecting", "client api connect server error");
			}

			nsent++;
			if((err = server->SendPacketStream(pk)) != 0)
			{
				if(server->autoReconnect && nsent <= 1 && (err==-ECONNRESET || err==-EPIPE))
				{
					resend = 1;
					continue;
				}
				return_err_res(err, "API::sending", "client api send packet error");
			}
		}

		NCResult *res = new NCResult(tdef);
		res->versionInfo.SetSerialNr(server->LastSerialNr());

		err = server->DecodeResultStream(*res);

		if(err < 0)
		{
			if(cmd!=DRequest::Nop && err==-EAGAIN && nsent<=1)
			{
				resend = 1;
				delete res;
				continue;
			}
			// network error always aborted
			return res;
		}
		nrecv++;

		if (res->versionInfo.SerialNr() != server->LastSerialNr())
		{
			log_debug("SN different, receive again. my SN: %lu, result SN: %lu",
					(unsigned long)server->LastSerialNr(),
					(unsigned long)res->versionInfo.SerialNr());
			// receive again
			resend = 0;
		} else if (res->ResultCode() == -EC_CHECKSUM_MISMATCH && nrecv <= 1)
		{
			resend = 1;
			// tabledef changed, resend
		}
		else
		{
			// got valid result
			if (res->ResultCode() >= 0 && cmd == DRequest::Get)
				res->SetVirtualMap(fs);
			return res;
		}
		delete res; // delete invalid result and loop
	}

	// UNREACHABLE
	return NULL;
}

NCResult *NCRequest::ExecuteDgram(CSocketAddress *peer, const CValue *kptr)
{
	int resend = 1;
	int nrecv = 0;
	int nsent = 0;

	while (1)
	{
		int err;

		if (resend) {
			CPacket pk;

			if ((err = Encode(kptr, &pk)) < 0)
			{
				return_err_res(err, "API::encoding", "client api encode error");
			}

			nsent++;
			if ((err = server->SendPacketDgram(peer, pk)) != 0)
			{
				if(peer==NULL && server->autoReconnect && nsent <= 1 &&
						(err==-ECONNRESET || err==-EPIPE || err==-ECONNREFUSED || err==-ENOTCONN))
				{
					if((err = server->Reconnect()) != 0) {
						if(err==-EAGAIN) {
							// unix socket return EAGAIN if listen queue overflow
							err = -EC_SERVER_BUSY;
						}
						return_err_res(err, "API::connecting", "client api connect server error");
					}
					resend = 1;
					continue;
				}
				return_err_res(err, "API::sending", "client api send packet error");
			}
		}

		NCResult *res = new NCResult(tdef);
		res->versionInfo.SetSerialNr(server->LastSerialNr());
		err = server->DecodeResultDgram(peer, *res);
		if(err < 0) {
			// network error encountered
			return res;
		}
		nrecv++;

		if (res->versionInfo.SerialNr() != server->LastSerialNr()) {
			log_debug("SN different, receive again. my SN: %lu, result SN: %lu",
					(unsigned long)server->LastSerialNr(),
					(unsigned long)res->versionInfo.SerialNr());
			// receive again
			resend = 0;
		} else if (res->ResultCode() == -EC_CHECKSUM_MISMATCH && nrecv <= 1) {
			resend = 1;
			// tabledef changed, resend
		} else {
			// got valid result
			if (res->ResultCode() >= 0 && cmd == DRequest::Get)
				res->SetVirtualMap(fs);
			return res;
		}
		delete res; // delete invalid result and loop
	}

	/* UNREACHABLE */
	return NULL;
}

NCResult *NCRequest::ExecuteNetwork(const CValue *kptr) {
	NCResult *res = NULL;
	if(!server->IsDgram())
	{
		res = ExecuteStream(kptr);
	}
	else if(server->IsDgram())
	{
		NCUdpPort *port = server->GetGlobalPort();
		//get UDPPORT{SN,fd} from udp pool
		if (port  == NULL)
		{
			return_err_res(err, "API::Execute", "get udp port error.udp port may exhaust");
		}
		res = ExecuteDgram(&server->addr, kptr);
		server->PutGlobalPort(port);
	}
	else
	{
		int err;
		if((err = server->Connect()) != 0)
		{
			if(err==-EAGAIN)
			{
				// unix socket return EAGAIN if listen queue overflow
				err = -EC_SERVER_BUSY;
			}
			return_err_res(err, "API::connecting", "client api connect server error");
		}

		res = ExecuteDgram(NULL, kptr);
	}
	return res;
}

NCResult *NCRequest::ExecuteInternal(const CValue *kptr)
{
	if(tdef == NULL) {
		return_err_res(err, "API::encoding", "internal error: request has no tdef");
	}

	int force = SetTabDef();

	int err;
	if(
			(err = ui.Resolve(tdef, force)) ||
			(err = ci.Resolve(tdef, force)) ||
			(err = fs.Resolve(tdef, force))
	  )
	{
		return_err_res(err, "API::encoding", "client api encode error");
	}

	NCResultInternal *res = server->ExecuteInternal(*this, kptr);

	if(res == NULL) {
		return_err_res(err, "API::sending", "internal client api execute error");
	}

	if(res->ResultCode() >= 0 && cmd==DRequest::Get)
		res->SetVirtualMap(fs);

	return reinterpret_cast<NCResult *>(res);
}

NCResult *NCRequest::PreCheck(const CValue *kptr) {

	if( cmd == DRequest::Monitor )
	{
		//Monitor Request need not check
		cmd = DRequest::SvrAdmin;//MonitorRequest is a stub of RequestSvrAdmin.Agent should add MonitorRequest type later
		return 	NULL;
	}
	if(err)
		return_err_res(err, "API::encoding", "Init Operation Error");
	if(server==NULL)
		return_err_res(-EC_NOT_INITIALIZED, "API::encoding", "Server Not Initialized");
	if(cmd==DRequest::ResultCode)
		return_err_res(-EC_BAD_COMMAND, "API::encoding", "Unknown Request Type");
	if(server->badkey)
		return_err_res(-EC_BAD_KEY_TYPE, "API::encoding", "Key Type Mismatch");
	if(server->badname)
		return_err_res(-EC_BAD_TABLE_NAME, "API::encoding", "Table Name Mismatch");
	if(cmd!=DRequest::Insert && cmd!=DRequest::SvrAdmin &&
			!(flags & DRequest::Flag::MultiKeyValue) && kptr==NULL)
		return_err_res(-EINVAL, "API::encoding", "Missing Key");
	if((flags & DRequest::Flag::MultiKeyValue) && kvl.IsFlat()==0)
		return_err_res(-EINVAL, "API::encoding", "Missing Key Value");
	if (tdef==NULL)
	{
        uint64_t time_before = GET_TIMESTAMP();
		server->Ping();//ping and get tabledef
        uint64_t time_after = GET_TIMESTAMP();
		tdef = server->tdef;

        uint64_t timeInterval = time_after - time_before;
        if(timeInterval< 0)
        	timeInterval = 0;
        //log_info("timeInterval: %lu", timeInterval);
        std::string accessKey = server->accessToken;
		server->dc->SetReportInfo(accessKey, timeInterval);
		log_info("NCRequest::PreCheck,seq:%lu, during:%lu", server->LastSerialNr(),timeInterval);
        //log_info("NCRequest::PreCheck, seq:%d,timeInterval:%lu", timeInterval);
	}
	return NULL;
}

int NCRequest::SetCompressFieldName()
{
    int iret = 0;
    if (tdef && tdef->CompressFieldId()>0)//启用压缩必定有这个字段
    {
        if (gzip==NULL)//读请求需要need字段
        {
            if (cmd != DRequest::Get)
                return 0;
            iret = Need(tdef->FieldName(tdef->CompressFieldId()),0);
            if (iret)
            {
                snprintf(_errmsg, sizeof(_errmsg), "need CompressField error,errorcode is %d",iret);
                return -1;
            }
        }
        else//写请求
		{
			if  (cmd == DRequest::Insert||cmd == DRequest::Replace)
			{
				iret = AddOperation(tdef->FieldName(tdef->CompressFieldId()),//name
						DField::Set,DField::Signed,CValue::Make(compressFlag));
			}
			else if (cmd == DRequest::Update)
			{
				iret = AddOperation(tdef->FieldName(tdef->CompressFieldId()),//name
						DField::OR,DField::Signed,CValue::Make(compressFlag));
			}
			else
			{
				 snprintf(_errmsg, sizeof(_errmsg), "Request type[%d] not support compressset",cmd);
				 return -1;
			}
            if (iret)
            {
                snprintf(_errmsg, sizeof(_errmsg), "set CompressField error,errorcode is %d",iret);
                return -1;
            }
        }

    }
    return 0;
}
//if get,need compressfield,if write set compressfield;
NCResult *NCRequest::Execute(const CValue *kptr) {
	NCResult *ret = NULL;
	if(kptr==NULL && haskey) kptr = &key;
	ret = PreCheck(kptr);
	if(ret==NULL)
	{
        if (SetCompressFieldName())
            return_err_res(-EC_COMPRESS_ERROR, "API::Execute", "SetCompressFieldName error. please check request.ErrorMessage");;

        /*date:2014/06/09, author:xuxinxin 模调上报 */
        uint64_t time_before = GET_TIMESTAMP();
        if(server->HasInternalExecutor())
			ret = ExecuteInternal(kptr);
		else
			ret = ExecuteNetwork(kptr);
        uint64_t time_after = GET_TIMESTAMP();
        uint64_t timeInterval = time_after - time_before;
        if(timeInterval< 0)
        	timeInterval = 0;
        std::string accessKey = server->accessToken;
		server->dc->SetReportInfo(accessKey, timeInterval);
		std::string stemp = accessKey.substr(0, 8);
		uint32_t bid = 0;
		sscanf(stemp.c_str(), "%u", &bid);
		log_info("NCRequest::Execute,seq:%lu, during:%lu, bid:%u", server->LastSerialNr(), timeInterval, bid);
	}
	else
		log_info("NCRequest::Execute,seq:%lu, PreCheck return NULL", server->LastSerialNr());
	return ret;
}

NCResult *NCRequest::Execute(int64_t k) {
	if(server == NULL)
	    return_err_res(-EC_NOT_INITIALIZED, "API::encoding", "Server Not Initialized");
	if(server->keytype != DField::Signed)
	    return_err_res(-EC_BAD_KEY_TYPE, "API::encoding", "Key Type Mismatch");
	CValue v(k);
	return Execute(&v);
}

NCResult *NCRequest::Execute(const char *k, int l) {
	if(server == NULL)
	    return_err_res(-EC_NOT_INITIALIZED, "API::encoding", "Server Not Initialized");
	if(server->keytype != DField::String)
	    return_err_res(-EC_BAD_KEY_TYPE, "API::encoding", "Key Type Mismatch");
	CValue v(k, l);
	return Execute(&v);
}

// packet encoding don't cache error code
int NCRequest::EncodeBuffer(char *&ptr, int &len, int64_t &magic, const CValue *kptr) {
	if(kptr==NULL && haskey) kptr = &key;
	int err = 0;
	NCResult *ret = PreCheck(kptr);
	if(ret!=NULL)
	{
		err = ret->ResultCode();
		delete ret;
		return err;
	}

	SetTabDef();
	if(tdef &&(
				(err = ui.Resolve(tdef, 0)) ||
				(err = ci.Resolve(tdef, 0)) ||
				(err = fs.Resolve(tdef, 0))
			  ))
	{
		return err;
	}

	if((err = CPacket::EncodeSimpleRequest(*this, kptr, ptr, len)) != 0) {
		return err;
	}

	magic = server->LastSerialNr();
	return 0;
}

int NCRequest::EncodeBuffer(char *&ptr, int &len, int64_t &magic, int64_t k) {
	if(server == NULL)
	    return -EC_NOT_INITIALIZED;
	if(server->keytype != DField::Signed)
	    return -EC_BAD_KEY_TYPE;
	if(cmd==DRequest::SvrAdmin)
	    return -EC_BAD_COMMAND;
	CValue v(k);
	return EncodeBuffer(ptr, len, magic, &v);
}

int NCRequest::EncodeBuffer(char *&ptr, int &len, int64_t &magic, const char *k, int l) {
	if(server == NULL)
	    return -EC_NOT_INITIALIZED;
	if(server->keytype != DField::String)
	    return -EC_BAD_KEY_TYPE;
	CValue v(k, l);
	return EncodeBuffer(ptr, len, magic, &v);
}

