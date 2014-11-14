#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <new>

#include "value.h"
#include "section.h"
#include "protocol.h"
#include "version.h"
#include "packet.h"
#include "../api/c_api/ttcint.h"
#include "tabledef.h"
#include "decode.h"

#include "log.h"

template<class T>
int TemplateEncodeRequest(NCRequest &rq, const CValue *kptr, T *tgt) {
	NCServer *sv = rq.server;
	int key_type = rq.keytype;
	const char* tab_name = rq.tablename;

	const char* accessKey = sv->accessToken.c_str();

//	int key_type = sv->keytype;
//	const char* tab_name = sv->tablename;

	CPacketHeader header;

	header.version = 1;
	header.scts = 8;
	header.flags = rq.tdef ? DRequest::Flag::KeepAlive :
		DRequest::Flag::KeepAlive + DRequest::Flag::NeedTableDefinition;
	header.flags |= (rq.flags & (DRequest::Flag::NoCache|DRequest::Flag::NoResult|DRequest::Flag::NoNextServer|DRequest::Flag::MultiKeyValue));
	header.cmd = rq.cmd;

	CVersionInfo vi;
	// tablename & hash
	vi.SetTableName(tab_name);
	if(rq.tdef) vi.SetTableHash(rq.tdef->TableHash());
	vi.SetSerialNr(sv->NextSerialNr());
	// app version
	if(sv->appname) vi.SetTag(5, sv->appname);
	// lib version
	vi.SetTag(6, "ctlib-v"TTC_VERSION);
	vi.SetTag(9, key_type);
	
	// hot backup id
	vi.SetHotBackupID(rq.hotBackupID);

	// hot backup timestamp
	vi.SetMasterHBTimestamp(rq.MasterHBTimestamp);
	vi.SetSlaveHBTimestamp(rq.SlaveHBTimestamp);
	if(sv->tdef && rq.adminCode != 0)
		vi.SetDataTableHash(sv->tdef->TableHash());
	
	// accessKey
	vi.SetAccessKey(accessKey);
	//log_info("accessKey:%s", vi.AccessKey().ptr);

	CArray kt(0, NULL);
	CArray kn(0, NULL);
	CArray kv(0, NULL);
	int isbatch = 0;
	if(rq.flags & DRequest::Flag::MultiKeyValue) {
		if(sv->SimpleBatchKey() && rq.kvl.KeyCount()==1) {
			/* single field single key batch, convert to normal */
			kptr = rq.kvl.val;
			header.flags &= ~DRequest::Flag::MultiKeyValue;
		} else {
			isbatch = 1;
		}
	}

	if(isbatch) {
		int keyFieldCnt = rq.kvl.KeyFields();
		int keyCount = rq.kvl.KeyCount();
		int i, j;
		vi.SetTag(10, keyFieldCnt);
		vi.SetTag(11, keyCount);
		// key type
		kt.ptr = (char*)MALLOC(sizeof(uint8_t)*keyFieldCnt);
		if(kt.ptr == NULL) throw std::bad_alloc();
		for(i=0; i<keyFieldCnt; i++)
			kt.Add((uint8_t)(rq.kvl.KeyType(i)));
		vi.SetTag(12, CValue::Make(kt.ptr, kt.len));
		// key name
		kn.ptr = (char*)MALLOC((256+sizeof(uint32_t))*keyFieldCnt);
		if(kn.ptr == NULL) throw std::bad_alloc();
		for(i=0; i<keyFieldCnt; i++)
			kn.Add(rq.kvl.KeyName(i));
		vi.SetTag(13, CValue::Make(kn.ptr, kn.len));
		// key value
		unsigned int buf_size = 0;
		for(j=0; j<keyCount; j++){
			for(i=0; i<keyFieldCnt; i++){
				CValue &v = rq.kvl(j, i);
				switch(rq.kvl.KeyType(i)){
					case DField::Signed:
					case DField::Unsigned:
						if(buf_size < kv.len + sizeof(uint64_t)){
							if(REALLOC(kv.ptr, buf_size+256) == NULL)
								throw std::bad_alloc();
							buf_size += 256;
						}
						kv.Add(v.u64);
						break;
					case DField::String:
					case DField::Binary:
						if(buf_size < (unsigned int)kv.len + sizeof(uint32_t) + v.bin.len){
							if(REALLOC(kv.ptr, buf_size+sizeof(uint32_t)+v.bin.len) == NULL)
								throw std::bad_alloc();
							buf_size += sizeof(uint32_t)+v.bin.len;
						}
						kv.Add(v.bin.ptr, v.bin.len);
						break;
					default:
						break;
				}
			}
		}
	}
	
	CRequestInfo ri;
	// key
	if(isbatch)
		ri.SetKey(CValue::Make(kv.ptr, kv.len));
	else if(kptr)
		ri.SetKey(*kptr);
	// cmd
	if(sv->GetTimeout()) {
	    ri.SetTimeout(sv->GetTimeout());
	}
	//limit
	if(rq.limitCount) {
	    ri.SetLimitStart(rq.limitStart);
	    ri.SetLimitCount(rq.limitCount);
	}
	if(rq.adminCode > 0){
		ri.SetAdminCode(rq.adminCode);
	}
	
	/* calculate version info */
	header.len[DRequest::Section::VersionInfo] =
	    EncodedBytesSimpleSection(vi, DField::None);

	//log_info("header.len:%d, vi.AccessKey:%s, vi.AccessKey().len:%d", header.len[DRequest::Section::VersionInfo], vi.AccessKey().ptr, vi.AccessKey().len);

	/* no table definition */
	header.len[DRequest::Section::TableDefinition] = 0;

	/* calculate rq info */
	header.len[DRequest::Section::RequestInfo] =
	    EncodedBytesSimpleSection(ri, isbatch?DField::String:key_type);

	/* no result info */
	header.len[DRequest::Section::ResultInfo] = 0;

	/* copy update info */
	header.len[DRequest::Section::UpdateInfo] =
	    EncodedBytesFieldValue(rq.ui);

	/* copy condition info */
	header.len[DRequest::Section::ConditionInfo] =
	    EncodedBytesFieldValue(rq.ci);

	/* full set */
	header.len[DRequest::Section::FieldSet] =
	    EncodedBytesFieldSet(rq.fs);

	/* no result set */
	header.len[DRequest::Section::ResultSet] = 0;

	const int len = CPacket::EncodeHeader(header);
	char *p = tgt->AllocateSimple(len);

	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	p = EncodeSimpleSection(p, vi, DField::None);
	p = EncodeSimpleSection(p, ri, isbatch?DField::String:key_type); 
	p = EncodeFieldValue(p, rq.ui);
	p = EncodeFieldValue(p, rq.ci);
	p = EncodeFieldSet(p, rq.fs);
	
	FREE(kt.ptr);
	FREE(kn.ptr);
	FREE(kv.ptr);
	
	return 0;
}

char * CPacket::AllocateSimple(int len) {
	buf = (CBufferChain *)MALLOC(sizeof(CBufferChain)+sizeof(struct iovec)+len);
	if(buf==NULL) throw std::bad_alloc();

	buf->nextBuffer = NULL;
    /* newman: pool, never use usedBytes here */
    buf->totalBytes = sizeof(struct iovec)+len;
	v = (struct iovec *)buf->data;
	nv = 1;
	char *p = buf->data + sizeof(struct iovec);
	v->iov_base = p;
	v->iov_len = len;
	bytes = len;
	return p;
}

int CPacket::EncodeRequest(NCRequest &rq, const CValue *kptr) {
	return TemplateEncodeRequest(rq, kptr, this);
}

class SimpleBuffer: public CBinary {
public:
	char * AllocateSimple(int size);
};

char * SimpleBuffer::AllocateSimple(int size) {
	len = size;
	ptr = (char *)MALLOC(len);
	if(ptr==NULL) throw std::bad_alloc();
	return ptr;
}

int CPacket::EncodeSimpleRequest(NCRequest &rq, const CValue *kptr, char * &ptr, int &len) {
	SimpleBuffer buf;
	int ret = TemplateEncodeRequest(rq, kptr, &buf);
	ptr = buf.ptr;
	len = buf.len;
	return ret;
}

