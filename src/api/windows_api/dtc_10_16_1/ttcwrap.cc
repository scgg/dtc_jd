#include "ttcapi.h"
#include "ttcint.h"
#include <stdio.h>
#include "protocol.h"
#include <stdint.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <windows.h>
using namespace TencentTableCache;






#define CAST(type, var) type *var = (type *)addr
#define CAST2(type, var, src) type *var = (type *)src->addr
#define CAST2Z(type, var, src) type *var = (type *)((src)?(src)->addr:0)
#define CAST3(type, var, src) type *var = (type *)src.addr


Server::Server(void) {
	WSADATA wsa={0};
	WSAStartup(MAKEWORD(2,2),&wsa);
		NCServer *srv = new NCServer;
		srv->INC();
		addr = srv;
}

Server::~Server(void) {
		CAST(NCServer, srv);
		int srvN=srv->DEC();
		if(srv && (srvN==0) )
		{
			delete srv;
			srv=0;
		}
		WSACleanup();
}



int Server::IntKey(void) {
		CAST(NCServer, srv);
		return srv->IntKey();
}


int Server::StringKey(void) {
		CAST(NCServer, srv);
		return srv->StringKey();
}


int Server::BinaryKey(void) {
		CAST(NCServer, srv);
		return srv->StringKey();
}


int Server::SetTableName(const char *name) {
		CAST(NCServer, srv);
		return srv->SetTableName(name);
}

int Server::SetAddress(const char *host, const char *port) {
	CAST(NCServer, srv);
	return srv->SetAddress(host, port);
}

void Server::SetTimeout(int n) {
	CAST(NCServer, srv);
	return srv->SetMTimeout(n*1000);
}

void Server::SetAccessKey(const char *token)
{
	CAST(NCServer, srv);
	srv->SetAccessKey(token);
}



Request::Request(Server *srv0, int op) {
		CAST2Z(NCServer, srv, srv0);
		NCRequest *req = new NCRequest(srv, op);
		addr = (void *)req;
}

Request::Request()
{
	NCRequest *req = new NCRequest(NULL, 0);
	addr = (void *)req;
}


Request::~Request(void) {
		CAST(NCRequest, req);
		delete req;
}

int Request::SetKey(long long key) {
	CAST(NCRequest, r); 
	return r->SetKey((int64_t)key);
}

int Request::SetKey(const char *key) {
		CAST(NCRequest, r);
		return r->SetKey(key, strlen(key));
}

int Request::SetKey(const char *key, int len) {
		CAST(NCRequest, r); 
		return r->SetKey(key, len);
}


int Request::Need(const char *name) {
		CAST(NCRequest, r); return r->Need(name, 0);
}


int Request::Need(const char *name, int vid) {
		CAST(NCRequest, r); return r->Need(name, vid);
}




Result *Request::Execute(void) {
		Result *s = new Result();
		CAST(NCRequest, r);
		s->addr = r->Execute();
		return s;
}


Result *Request::Execute(long long k) {
		Result *s = new Result();
		CAST(NCRequest, r);
		s->addr = r->Execute((int64_t)k);
		return s;
}


Result *Request::Execute(const char *k) {
		Result *s = new Result();
		CAST(NCRequest, r);
		s->addr = r->Execute(k, k?strlen(k):0);
		return s;
}


Result *Request::Execute(const char *k, int l) {
		Result *s = new Result();
		CAST(NCRequest, r);
		s->addr = r->Execute(k, l);
		return s;
}


int Request::Execute(Result &s) {
		CAST(NCRequest, r);
		s.Reset();
		s.addr = r->Execute();
		return s.ResultCode();
}


int Request::Execute(Result &s, long long k) {
		CAST(NCRequest, r);
		s.Reset();
		s.addr = r->Execute((int64_t)k);
		return s.ResultCode();
}


int Request::Execute(Result &s, const char *k) {
		CAST(NCRequest, r);
		s.Reset();
		s.addr = r->Execute(k, k?strlen(k):0);
		return s.ResultCode();
}


int Request::Execute(Result &s, const char *k, int l) {
		CAST(NCRequest, r);
		s.Reset();
		s.addr = r->Execute(k, l);
		return s.ResultCode();
}





Result::Result(void) {
	addr = NULL;
}


Result::~Result(void) {
		Reset();
}

void Result::Reset(void) {
	if(addr != NULL) {
		CAST(NCResult, task);
		CAST(NCResultInternal, itask);
		if(task->Role()==TaskRoleClient)
			delete task;
		else
			delete itask;
	}
	addr = NULL;
}

int Result::ResultCode(void) const {
	CAST(NCResult, r);
	if(r==NULL) return 0;
	return r->ResultCode();
}

int Result::NumRows(void) const {
	CAST(NCResult, r);
	if(r==NULL || r->result==NULL) return 0;
	return r->result->TotalRows();
}

int Result::FetchRow(void) {
	CAST(NCResult, r);
	if(r==NULL) return -EC_NO_MORE_DATA;
	if(r->ResultCode()<0) return r->ResultCode();
	if(r->result==NULL) return -EC_NO_MORE_DATA;
	return r->result->DecodeRow();
}

const char *Result::BinaryValue(const char *name) const {
	return BinaryValue(name, NULL);
}


static inline const char *GetBinaryValue(NCResult *r, int id, int *lenp) {
	if(id >= 0) {
		const CValue *v;
		if(id==0 && !(r->result->FieldPresent(0)))
			v = r->ResultKey();
		else
			v = r->result->FieldValue(id);

		if(v) {
			switch(r->FieldType(id)) {
			case DField::String:
			case DField::Binary:
				if(lenp) *lenp = v->bin.len;
				return v->str.ptr;
			}
		}
	}
	return NULL;
}


const char *Result::BinaryValue(const char *name, int *lenp) const {
	if(lenp) *lenp = 0;
	CAST(NCResult, r);
	if(r && r->result)
		return GetBinaryValue(r, r->FieldId(name), lenp);
	return NULL;
}


static inline int64_t GetIntValue(NCResult *r, int id) {
	if(id >= 0) {
		const CValue *v;
		if(id==0 && !(r->result->FieldPresent(0)))
			v = r->ResultKey();
		else
			v = r->result->FieldValue(id);

		if(v) {
			switch(r->FieldType(id)) {
			case DField::Signed:
			case DField::Unsigned:
				return v->s64;
			case DField::Float:
				return (long long)(v->flt);
				//return (int64_t)(v->flt);
			case DField::String:
				if(v->str.ptr)
					return atol(v->str.ptr);
			}
		}
	}
	return 0;
}


long long Result::IntValue(const char *name) const {
	CAST(NCResult, r);
	if(r && r->result)
		return GetIntValue(r, r->FieldId(name));
	return 0;
}

int Result::AffectedRows(void) const {
	CAST(NCResult, r);
	if(r==NULL) return 0;
	return r->resultInfo.AffectedRows();
}

#define _DECLACT_(f0,t0,f1,f2,t1) int Request::f0(const char *n, t0 v){ CAST(NCRequest, r); return r->f1(n, DField::f2, DField::t1, CValue::Make(v)); } 
#define _DECLACT2_(f0,t0,f1,f2,t1) int Request::f0(const char *n, t0 v, int l){ CAST(NCRequest, r); return r->f1(n, DField::f2, DField::t1, CValue::Make(v,l)); } 

_DECLACT_(EQ, long long, AddCondition, EQ, Signed);
_DECLACT_(NE, long long, AddCondition, NE, Signed);
_DECLACT_(GT, long long, AddCondition, GT, Signed);
_DECLACT_(GE, long long, AddCondition, GE, Signed);
_DECLACT_(LT, long long, AddCondition, LT, Signed);
_DECLACT_(LE, long long, AddCondition, LE, Signed);
_DECLACT_(EQ, const char *, AddCondition, EQ, String);
_DECLACT_(NE, const char *, AddCondition, NE, String);
_DECLACT2_(EQ, const char *, AddCondition, EQ, String);
_DECLACT2_(NE, const char *, AddCondition, NE, String);

_DECLACT_(OR, long long, AddOperation, OR, Signed);
_DECLACT_(Add, long long, AddOperation, Add, Signed);
_DECLACT_(Add, double, AddOperation, Add, Float);

_DECLACT_(Set, long long, AddOperation, Set, Signed);
_DECLACT_(Set, double, AddOperation, Set, Float);
_DECLACT_(Set, const char *, AddOperation, Set, String);
_DECLACT2_(Set, const char *, AddOperation, Set, String);





int Request::Sub(const char *n, long long v) {
	CAST(NCRequest, r);
	return r->AddOperation(n, DField::Add, DField::Signed, CValue::Make(-(int64_t)v));
}

int Request::Sub(const char *n, double v) {
	CAST(NCRequest, r);
	return r->AddOperation(n, DField::Add, DField::Signed, CValue::Make(-v));
}


int Request::SetMultiBits(const char *n, int o, int s, unsigned int v) {
	if(s <=0 || o >= (1 << 24) || o < 0)
		return 0;
	if(s >= 32)
		s = 32;

	CAST(NCRequest, r);
	// 1 byte 3 byte 4 byte
	//   s     o      v
	uint64_t t = ((uint64_t)s << 56)+ ((uint64_t)o << 32) + v;
	return r->AddOperation(n, DField::SetBits, DField::Unsigned, CValue::Make(t));
}


int Request::AddKeyValue(const char* name, long long v) {
	CAST(NCRequest, r);
	CValue key(v);
	return r->AddKeyValue(name, key, KeyTypeInt);
}

int Request::AddKeyValue(const char* name, const char *v) {
	CAST(NCRequest, r);
	CValue key(v);
	return r->AddKeyValue(name, key, KeyTypeString);
}

int Request::AddKeyValue(const char* name, const char *v, int len) {
	CAST(NCRequest, r);
	CValue key(v, len);
	return r->AddKeyValue(name, key, KeyTypeString);
}



int Request::EncodePacket(char *&ptr, int &len, long long &m) {
		CAST(NCRequest, r);
		int64_t mm;
		int ret = r->EncodeBuffer(ptr, len, mm);
		m = mm;
		return ret;
}


int Request::EncodePacket(char *&ptr, int &len, long long &m, long long k) {
		CAST(NCRequest, r);
		int64_t mm;
		int ret = r->EncodeBuffer(ptr, len, mm, (int64_t)k);
		m = mm;
		return ret;
}


int Request::EncodePacket(char *&ptr, int &len, long long &m, const char *k) {
		CAST(NCRequest, r);
		int64_t mm;
		int ret = r->EncodeBuffer(ptr, len, mm, k, k?strlen(k):0);
		m = mm;
		return ret;
}


int Request::EncodePacket(char *&ptr, int &len, long long &m, const char *k, int l) {
		CAST(NCRequest, r);
		int64_t mm;
		int ret = r->EncodeBuffer(ptr, len, mm, k, l);
		m = mm;
		return ret;
}


int Request::SetCacheID(long long id){
	CAST(NCRequest, r); return r->SetCacheID(id);
}


static inline const char *GetStringValue(NCResult *r, int id, int *lenp) {
	if(id >= 0) {
		const CValue *v;
		if(id==0 && !(r->result->FieldPresent(0)))
			v = r->ResultKey();
		else
			v = r->result->FieldValue(id);

		if(v) {
			switch(r->FieldType(id)) {
			case DField::String:
				if(lenp) *lenp = v->bin.len;
				return v->str.ptr;
			}
		}
	}
	return NULL;
}




const char *Result::StringValue(const char *name, int *lenp) const {
	if(lenp) *lenp = 0;
	CAST(NCResult, r);
	if(r && r->result)
		return GetStringValue(r, r->FieldId(name), lenp);
	return NULL;
}


const char *Result::StringValue(int id, int *lenp) const {
		if(lenp) *lenp = 0;
		CAST(NCResult, r);
		if(r && r->result)
			return GetStringValue(r, r->FieldIdVirtual(id), lenp);
		return NULL;
}


const char *Result::StringValue(const char *name, int &len) const {
		return StringValue(name, &len);
}


const char *Result::StringValue(int id, int &len) const {
		return StringValue(id, &len);
}


const char *Result::StringValue(const char *name) const {
		return StringValue(name, NULL);
}


const char *Result::StringValue(int id) const {
		return StringValue(id, NULL);
}