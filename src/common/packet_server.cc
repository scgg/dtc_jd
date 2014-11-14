#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <new>

#include "version.h"
#include "packet.h"
#include "tabledef.h"
#include "decode.h"
#include "task_request.h"

#include "log.h"

/* not yet pollized*/
int CPacket::EncodeDetect(const CTableDefinition *tdef, int sn)
{
	CPacketHeader header;

	header.version = 1;
	header.scts = 8;
	header.flags = DRequest::Flag::KeepAlive;
	header.cmd = DRequest::Get;

	CVersionInfo vi;
	// tablename & hash
	vi.SetTableName(tdef->TableName());
	vi.SetTableHash(tdef->TableHash());	
	vi.SetSerialNr(sn);
	// app version
	vi.SetTag(5, "ttcd");
	// lib version
	vi.SetTag(6, "ctlib-v"TTC_VERSION);
	vi.SetTag(9, tdef->FieldType(0));

	CRequestInfo ri;
	// key
	ri.SetKey(CValue::Make(0));
//	ri.SetTimeout(30);
	
	// field set
	char fs[4]={1, 0, 0, 0xFF};
	
	/* calculate version info */
	header.len[DRequest::Section::VersionInfo] =
		EncodedBytesSimpleSection(vi, DField::None);

	/* no table definition */
	header.len[DRequest::Section::TableDefinition] = 0;

	/* encode request info */
	header.len[DRequest::Section::RequestInfo] = 
		EncodedBytesSimpleSection(ri, tdef->KeyType());

	/* no result info */
	header.len[DRequest::Section::ResultInfo] = 0;

	/* encode update info */
	header.len[DRequest::Section::UpdateInfo] = 0;

	/* encode condition info */
	header.len[DRequest::Section::ConditionInfo] = 0;
	
	/* full set */
	header.len[DRequest::Section::FieldSet] = 4;

	/* no result set */
	header.len[DRequest::Section::ResultSet] = 0;

	bytes = EncodeHeader(header);
	const int len = bytes;

	/* newman: pool, exist and large enough, use. else free and malloc */
	int total_len = sizeof(CBufferChain)+sizeof(struct iovec)+len;
	if(buf == NULL)
	{
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain); 
	}else if(buf && buf->totalBytes < (int)(total_len - sizeof(CBufferChain)))
	{
	    FREE_IF(buf);
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		 return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain);
	}

    /* usedBtytes never used for CPacket's buf */
	buf->nextBuffer = NULL;
	v = (struct iovec *)buf->data;
	nv = 1;
	char *p = buf->data + sizeof(struct iovec);
	v->iov_base = p;
	v->iov_len = len;

	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	p = EncodeSimpleSection(p, vi, DField::None);
	p = EncodeSimpleSection(p, ri, tdef->KeyType());
	
	// encode field set
	memcpy(p, fs, 4);
	p += 4;

	if(p-(char *)v->iov_base != len)
		fprintf(stderr, "%s(%d): BAD ENCODER len=%ld must=%d\n",
			__FILE__, __LINE__, (long)(p - (char *)v->iov_base), len);

	return 0;
}

static char *EncodeBinary(char *p, const char *src, int len) {
	if(len) memcpy(p, src, len);
	return p + len;
}

static char *EncodeBinary(char *p, const CBinary &b) {
	return EncodeBinary(p, b.ptr, b.len);
}


int CPacket::EncodeFetchData(CTaskRequest &task)
{
	const CTableDefinition *tdef = task.TableDefinition();
	CPacketHeader header;

	header.version = 1;
	header.scts = 8;
	header.flags = DRequest::Flag::KeepAlive;
	header.cmd = DRequest::Get;

	// save & remove limit information
	uint32_t limitStart = task.requestInfo.LimitStart();
	uint32_t limitCount = task.requestInfo.LimitCount();
	task.requestInfo.SetLimitStart(0);
	task.requestInfo.SetLimitCount(0);

	/* calculate version info */
	header.len[DRequest::Section::VersionInfo] =
		EncodedBytesSimpleSection(task.versionInfo, DField::None);

	/* no table definition */
	header.len[DRequest::Section::TableDefinition] = 0;

	/* encode request info */
	header.len[DRequest::Section::RequestInfo] = 
		EncodedBytesSimpleSection(task.requestInfo, tdef->KeyType());

	/* no result info */
	header.len[DRequest::Section::ResultInfo] = 0;

	/* no update info */
	header.len[DRequest::Section::UpdateInfo] = 0;

	/* encode condition info */
	header.len[DRequest::Section::ConditionInfo] = 
		EncodedBytesMultiKey(task.MultiKeyArray(), task.TableDefinition());

	/* full set */
	header.len[DRequest::Section::FieldSet] =
		tdef->PackedFieldSet(task.FlagFieldSetWithKey()).len;

	/* no result set */
	header.len[DRequest::Section::ResultSet] = 0;

	bytes = EncodeHeader(header);
	const int len = bytes;
    
	/* newman: pool, exist and large enough, use. else free and malloc */
	int total_len = sizeof(CBufferChain)+sizeof(struct iovec)+len;
	if(buf == NULL)
	{
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain); 
	}else if(buf && buf->totalBytes < total_len - (int)sizeof(CBufferChain))
	{
	    FREE_IF(buf);
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		 return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain);
	}

	buf->nextBuffer = NULL;
	v = (struct iovec *)buf->data;
	nv = 1;
	char *p = buf->data + sizeof(struct iovec);
	v->iov_base = p;
	v->iov_len = len;

	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	p = EncodeSimpleSection(p, task.versionInfo, DField::None);
	p = EncodeSimpleSection(p, task.requestInfo, tdef->KeyType());
	// restore limit info
	task.requestInfo.SetLimitStart(limitStart);
	task.requestInfo.SetLimitCount(limitCount);
	p = EncodeMultiKey(p, task.MultiKeyArray(), task.TableDefinition());
	p = EncodeBinary(p, tdef->PackedFieldSet(task.FlagFieldSetWithKey()));

	if(p-(char *)v->iov_base != len)
		fprintf(stderr, "%s(%d): BAD ENCODER len=%ld must=%d\n",
			__FILE__, __LINE__, (long)(p - (char *)v->iov_base), len);

	return 0;
}

int CPacket::EncodePassThru(CTask &task)
{
	const CTableDefinition *tdef = task.TableDefinition();
	CPacketHeader header;

	header.version = 1;
	header.scts = 8;
	header.flags = DRequest::Flag::KeepAlive;
	header.cmd = task.RequestCode();

	/* calculate version info */
	header.len[DRequest::Section::VersionInfo] =
		EncodedBytesSimpleSection(task.versionInfo, DField::None);

	/* no table definition */
	header.len[DRequest::Section::TableDefinition] = 0;

	/* encode request info */
	header.len[DRequest::Section::RequestInfo] = 
		EncodedBytesSimpleSection(task.requestInfo, tdef->KeyType());

	/* no result info */
	header.len[DRequest::Section::ResultInfo] = 0;

	/* encode update info */
	header.len[DRequest::Section::UpdateInfo] =
		task.RequestOperation() ?  EncodedBytesFieldValue(*task.RequestOperation()) : 0;

	/* encode condition info */
	header.len[DRequest::Section::ConditionInfo] = 
		task.RequestCondition() ? EncodedBytesFieldValue(*task.RequestCondition()) : 0;

	/* full set */
	header.len[DRequest::Section::FieldSet] =
		task.RequestFields() ? EncodedBytesFieldSet(*task.RequestFields()) : 0;

	/* no result set */
	header.len[DRequest::Section::ResultSet] = 0;

	bytes = EncodeHeader(header);
	const int len = bytes;
    
	/* newman: pool, exist and large enough, use. else free and malloc */
	int total_len = sizeof(CBufferChain)+sizeof(struct iovec)+len;
	if(buf == NULL)
	{
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain); 
	}else if(buf && buf->totalBytes < total_len - (int)sizeof(CBufferChain))
	{
	    FREE_IF(buf);
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		 return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain);
	}

	buf->nextBuffer = NULL;
	v = (struct iovec *)buf->data;
	nv = 1;
	char *p = buf->data + sizeof(struct iovec);
	v->iov_base = p;
	v->iov_len = len;

	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	p = EncodeSimpleSection(p, task.versionInfo, DField::None);
	p = EncodeSimpleSection(p, task.requestInfo, tdef->KeyType());
	if(task.RequestOperation()) p = EncodeFieldValue(p, *task.RequestOperation());
	if(task.RequestCondition()) p = EncodeFieldValue(p, *task.RequestCondition());
	if(task.RequestFields()) p = EncodeFieldSet(p, *task.RequestFields());

	if(p-(char *)v->iov_base != len)
		fprintf(stderr, "%s(%d): BAD ENCODER len=%ld must=%d\n",
			__FILE__, __LINE__, (long)(p - (char *)v->iov_base), len);

	return 0;
}

int CPacket::EncodeForwardRequest(CTaskRequest &task)
{
	if(task.FlagPassThru())
		return EncodePassThru(task);
	if(task.FlagFetchData())
		return EncodeFetchData(task);
	if(task.RequestCode() == DRequest::Get)
		return EncodeFetchData(task);
	return EncodePassThru(task);
}

int CPacket::EncodeResult(CTask &task, int mtu, uint32_t ts)
{
	const CTableDefinition *tdef = task.TableDefinition();
	CResultPacket *rp = task.ResultCode() >= 0 ? task.GetResultPacket() : NULL;
	CBufferChain *rb = NULL;
	int nrp=0, lrp=0, off = 0;

	if(mtu <= 0) {
		mtu = MAXPACKETSIZE;
	}

	/* rp may exist but no result */
	if(rp && (rp->numRows||rp->totalRows)) {
	    rb = rp->bc;
	    if(rb) rb->Count(nrp, lrp);
	    off = 5 - EncodedBytesLength(rp->numRows);
	    EncodeLength(rb->data+off, rp->numRows);
	    lrp -= off;
	    task.resultInfo.SetTotalRows(rp->totalRows);
	} else {
	    if(rp && rp->totalRows == 0 && rp->bc)
	    {
		FREE(rp->bc);
		rp->bc = NULL;
	    }
	    task.resultInfo.SetTotalRows(0);
	    if(task.ResultCode()==0)
		 task.SetError(0, NULL, NULL);
	}
	if(ts) {
		task.resultInfo.SetTimestamp(ts);
	}
	task.versionInfo.SetSerialNr(task.RequestSerial());
	task.versionInfo.SetTag(6, "ctlib-v"TTC_VERSION);
	if(task.ResultKey()==NULL && task.RequestKey()!=NULL)
		task.SetResultKey(*task.RequestKey());
	
	CPacketHeader header;

	header.version = 1;
	header.scts = 8;
	header.flags = DRequest::Flag::KeepAlive|task.FlagMultiKeyVal();
	/* rp may exist but no result */
	header.cmd = (rp && (rp->numRows||rp->totalRows)) ? DRequest::ResultSet : DRequest::ResultCode;

	/* calculate version info */
	header.len[DRequest::Section::VersionInfo] =
		EncodedBytesSimpleSection(task.versionInfo, DField::None);

	/* copy table definition */
	header.len[DRequest::Section::TableDefinition] =
			task.FlagTableDefinition() ?
			tdef->PackedDefinition().len : 0;

	/* no request info */
	header.len[DRequest::Section::RequestInfo] = 0;

	/* calculate result info */
	header.len[DRequest::Section::ResultInfo] =
	    EncodedBytesSimpleSection(task.resultInfo,
	    	tdef->FieldType(0));

	/* no update info */
	header.len[DRequest::Section::UpdateInfo] = 0;

	/* no condition info */
	header.len[DRequest::Section::ConditionInfo] = 0;

	/* no field set */
	header.len[DRequest::Section::FieldSet] = 0;

	/* copy result set */
	header.len[DRequest::Section::ResultSet] = lrp;

	bytes = EncodeHeader(header);
	if(bytes > mtu) {
		/* clear result set */
		nrp = 0;
		lrp = 0;
		rb = NULL;
		rp = NULL;
		/* set message size error */
		task.SetError(-EMSGSIZE, "EncodeResult", "encoded result exceed the maximum network packet size");
		/* re-encode resultinfo */
		header.len[DRequest::Section::ResultInfo] =
			EncodedBytesSimpleSection(task.resultInfo,
					tdef->FieldType(0));
		header.cmd = DRequest::ResultCode;
		header.len[DRequest::Section::ResultSet] = 0;
		/* FIXME: only work in LITTLE ENDIAN machine */
		bytes = EncodeHeader(header);
	}

	//non-result packet len
	const int len = bytes - lrp;
    
	/* newman: pool, exist and large enough, use. else free and malloc */
	int total_len = sizeof(CBufferChain)+sizeof(struct iovec)*(nrp+1)+len;
	if(buf == NULL)
	{
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain); 
	}else if(buf && buf->totalBytes < total_len - (int)sizeof(CBufferChain))
	{
	    FREE_IF(buf);
	    buf = (CBufferChain *)MALLOC(total_len);
	    if(buf==NULL)
	    {
		 return -ENOMEM;
	    }
        buf->totalBytes = total_len - sizeof(CBufferChain);
	}

	/*
	buf = (CBufferChain *)MALLOC(sizeof(CBufferChain)+sizeof(struct iovec)*(nrp+1)+len);
	if(buf==NULL) return -ENOMEM;
	*/

	buf->nextBuffer = nrp ? rb : NULL;
	v = (struct iovec *)buf->data;
	char *p = buf->data + sizeof(struct iovec)*(nrp+1);
	v->iov_base = p;
	v->iov_len = len;
	nv = nrp +1;
	
	for(int i=1; i<=nrp; i++, rb=rb->nextBuffer) {
		v[i].iov_base = rb->data + off;
		v[i].iov_len = rb->usedBytes - off;
		off = 0;
	}

    /* save rp's bc */
	//if(rp) rp->bc = NULL;

	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	p = EncodeSimpleSection(p, task.versionInfo, DField::None);
	if(task.FlagTableDefinition())
		p = EncodeBinary(p, tdef->PackedDefinition());
	p = EncodeSimpleSection(p, task.resultInfo, tdef->FieldType(0));

	if(p-(char *)v->iov_base != len)
		fprintf(stderr, "%s(%d): BAD ENCODER len=%ld must=%d\n",
			__FILE__, __LINE__, (long)(p - (char *)v->iov_base), len);

	return 0;
}

int CPacket::EncodeResult(CTaskRequest &task, int mtu)
{
	return EncodeResult( (CTask&)task, mtu, task.Timestamp());
}

void CPacket::FreeResultBuff()
{
        CBufferChain * resbuff = buf->nextBuffer;
	buf->nextBuffer = NULL;

	while(resbuff) {
              char *p = (char *)resbuff;
               resbuff = resbuff->nextBuffer;
               FREE(p);
        }    
}

int CPacket::Bytes(void) 
{
	int sendbytes = 0;
	for(int i = 0; i < nv; i++)
	{
		sendbytes += v[i].iov_len;
	} 
	return sendbytes;
}
