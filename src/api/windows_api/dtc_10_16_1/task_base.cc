#include "task_base.h"
#include "indecode.h"

typedef unsigned int uint_32 ;  
uint_32 bswap_32(uint_32 x)  
{  
	return (((uint_32)(x) & 0xff000000) >> 24) |   
		(((uint_32)(x) & 0x00ff0000) >> 8) | 
		(((uint_32)(x) & 0x0000ff00) << 8) |   
		(((uint_32)(x) & 0x000000ff) << 24) ;  
}


void CTask::DecodeStream(CSimpleReceiver &receiver) 
{
	int rv;

	switch(stage) 
	{
	default:
		break;
	case DecodeStageFatalError:
	case DecodeStageDataError:
	case DecodeStageDone:
		return;

	case DecodeStageIdle:
		receiver.init();
	case DecodeStageWaitHeader:
		rv = receiver.fill();

		if(rv==-1) 
		{
			if(errno != EAGAIN && errno != EINTR && errno != EINPROGRESS)
				stage = DecodeStageFatalError;
			return;
		}

		if(rv==0) 
		{
			errno = role==TaskRoleServer && stage==DecodeStageIdle ? 0 : ECONNRESET;
			stage = DecodeStageFatalError;
			return;
		}

		stage = DecodeStageWaitHeader;

		if(receiver.remain() > 0)
		{
			return;
		}

		if((rv = DecodeHeader(receiver.header(), &receiver.header())) < 0)
		{
			return;
		}

		//		printf("request body: %d, header: %d\n", (int)rv, (int)sizeof(struct CPacketHeader));

		char *buf = packetbuf.Allocate(rv, role);
		receiver.set(buf, rv);
		if(buf==NULL) 
		{
			printf("decoder Insufficient Memory\n");
			stage = DecodeStageDiscard;
		}
	}

	if(stage == DecodeStageDiscard) 
	{
		rv = receiver.discard();
		if(rv==-1) 
		{
			if(errno != EAGAIN && errno != EINTR)
			{
				stage = DecodeStageFatalError;
			}

			return;
		}

		if(rv==0) 
		{
			stage = DecodeStageFatalError;
			return;
		}

		stage = DecodeStageDataError;
		return;
	}

	rv = receiver.fill();

	if(rv==-1) 
	{
		if(errno != EAGAIN && errno != EINTR)
		{
			stage = DecodeStageFatalError;
		}

		return;
	}

	if(rv==0) 
	{
		stage = DecodeStageFatalError;
		return;
	}

	if(receiver.remain() <= 0)
	{
		DecodeRequest(receiver.header(), receiver.c_str());
	}

	return;
}



int CTask::DecodeHeader(const CPacketHeader &header, CPacketHeader *out) 
{

	if(header.version != 1) 
	{ // version not supported
		stage = DecodeStageFatalError;
		printf("version incorrect: %d", header.version);

		return -1;
	}

	if(header.scts != DRequest::Section::Total) 
	{ // tags# mismatch
		stage = DecodeStageFatalError;
		printf("session count incorrect: %d", header.scts);

		return -2;
	}

	int i;
	int64_t n;
	for(i=0, n=0; i<DRequest::Section::Total; i++) 
	{
		const unsigned int v = header.len[i];
		n += v;
	}

	if(n > MAXPACKETSIZE) 
	{ // oversize
		stage = DecodeStageFatalError;
		printf("package size error: %ld", (long)n);

		return -4;
	}

	if(header.cmd==DRequest::ResultCode||header.cmd==DRequest::ResultSet) {
		replyCode = header.cmd;
		replyFlags = header.flags;
	}
	else
	{
		requestCode = header.cmd;
		requestFlags = header.flags;
		requestType = cmd2type[requestCode];
	}

	stage = DecodeStageWaitData;

	return (int)n;
}



void CTask::DecodeRequest(CPacketHeader &header, char *p)
{
	int id = 0;
	int err = 0;
	
	if(header.len[id]) 
	{
	    /* decode version info */
	    err = DecodeSimpleSection(p, header.len[id], versionInfo, DField::None);
	    if(err) printf("decode version info error, decode version info error: %d \n", err);
		
	    /* backup serialNr */
	    serialNr = versionInfo.SerialNr();

	    if(header.cmd == DRequest::SvrAdmin)
	    { 
	//	SetTableDefinition(hotbackupTableDef); // ����������ɹ������
	    }

	    if(role==TaskRoleServer)
	    {		

		/* checking tablename */
		if(!IsSameTable(versionInfo.TableName()))
		{
		    err = -EC_TABLE_MISMATCH;
		    requestFlags |= DRequest::Flag::NeedTableDefinition;

		    printf("table mismatch,table mismatch: %d, client[%.*s], server[%s]", err,
			    versionInfo.TableName().len,
			    versionInfo.TableName().ptr,
			    TableName());
		}

		/* check table hash */
		if( versionInfo.TableHash().len > 0 && !HashEqual(versionInfo.TableHash()))
		{
		    err = -EC_CHECKSUM_MISMATCH;
		    requestFlags |= DRequest::Flag::NeedTableDefinition;

		    printf("table mismatch, table mismatch: %d", err);
		}
		/* checking keytype */
		const unsigned int t = versionInfo.KeyType();
		const int rt = KeyType();
		if(!(requestFlags & DRequest::Flag::MultiKeyValue) && (t >= DField::TotalType || !validktype[t][rt])) {
		    err = -EC_BAD_KEY_TYPE;
		    requestFlags |= DRequest::Flag::NeedTableDefinition;

		    printf("key type incorrect, key type incorrect: %d", err);
		}
	    }

	}//end of version info

	if(ValidateSection(header) < 0)
	{
	    stage = DecodeStageDataError;
	    return;
	}

	if(header.cmd==DRequest::Nop && role==TaskRoleServer) 
	{
		stage = DecodeStageDataError;
		return;
	}

	p += header.len[id++];

	// only client use remote table
	if(header.len[id] && AllowRemoteTable())
	{
	    	/* decode table definition */
	    	CTableDefinition *tdef;
		try { tdef = new CTableDefinition; }
		catch (int e) { err = e; goto error; }
		int rv = tdef->Unpack(p, header.len[id]);
		if(rv != 0) {
			delete tdef;

			printf("unpack table info error,unpack table info error: %d", rv);
		}
		CTableDefinition *old = TableDefinition();
		DEC_DELETE(old);
		SetTableDefinition(tdef);
		tdef->INC();
		if(!(header.flags & DRequest::Flag::AdminTable))
		    MarkHasRemoteTable();
	}

	p += header.len[id++];

	if(header.len[id])
	{
	    /* decode request info */
		if(requestFlags & DRequest::Flag::MultiKeyValue)
			err = DecodeSimpleSection(p, header.len[id], requestInfo,
				DField::Binary);
		else
			err = DecodeSimpleSection(p, header.len[id], requestInfo,
				TableDefinition() ? FieldType(0): DField::None);

	    if(err) printf("decode request info error, decode request info error: %d", err);
		
	    /* key present */
	    key = requestInfo.Key();

		if(header.cmd == DRequest::SvrAdmin && requestInfo.AdminCode() == DRequest::ServerAdminCmd::RegisterHB){
		    // ע���ȱ���ϵ��ҪУ���ṹ�Ƿ�һ��
			/* checking tablename */
			/*
			if(!dataTableDef->IsSameTable(versionInfo.TableName()))
			{
			    err = -EC_TABLE_MISMATCH;
			    requestFlags |= DRequest::Flag::NeedTableDefinition;

				ERR_RET("table mismatch", "table mismatch: %d", err);
			}
			*/
			/* check table hash */
			/*
			if(versionInfo.TableHash().len <= 0 || !dataTableDef->HashEqual(versionInfo.TableHash()))
			{
			    err = -EC_CHECKSUM_MISMATCH;
			    requestFlags |= DRequest::Flag::NeedTableDefinition;

				ERR_RET("table mismatch", "table mismatch: %d", err);
			}
			*/
			if(versionInfo.DataTableHash().len <= 0 || !dataTableDef->HashEqual(versionInfo.DataTableHash()))
			{
			    err = -EC_CHECKSUM_MISMATCH;
			    requestFlags |= DRequest::Flag::NeedTableDefinition;

				printf("table mismatch, table mismatch: %d\n", err);
			}
		}
	}

	p += header.len[id++];

	if(header.len[id]) {
	    /* decode result code */
	    err = DecodeSimpleSection(p, header.len[id], resultInfo,
		    TableDefinition() ? FieldType(0): DField::None);

	    if(err) printf("decode result info error, decode result info error: %d\n", err);
	    rkey = resultInfo.Key();
	}

	p += header.len[id++];

	if(header.len[id]) {
	    /* decode updateInfo */
	    const int mode = requestCode==DRequest::Update ? 2 : 1;
	    err = DecodeFieldValue(p, header.len[id], mode);

	    if(err) printf("decode update info error, decode update info error: %d\n", err);
	}

	p += header.len[id++];

	if(header.len[id]) {
	    /* decode conditionInfo */
	    err = DecodeFieldValue(p, header.len[id], 0);

        if(err) printf("decode condition error, decode condition error: %d\n", err);
	}

	p += header.len[id++];

	if(header.len[id]) {
	    /* decode fieldset */
	    err = DecodeFieldSet(p, header.len[id]);

	    if(err) printf("decode field set error, decode field set error: %d\n", err);
	}

	p += header.len[id++];

	if(header.len[id]) {
	    /* decode resultset */
	    err = DecodeResultSet(p, header.len[id]);

	    if(err) printf("decode result set error, decode result set error: %d\n", err);
	}

	stage = DecodeStageDone;
	return;

#undef ERR_RET

error:
	printf("decoder,%d\n", err);
	stage = DecodeStageDataError;
}



int CTask::ValidateSection(CPacketHeader &header) 
{
	int i;
	int m;
	for(i=0, m=0; i<DRequest::Section::Total; i++) 
	{
		if(header.len[i])
			m |= 1<<i;
	}

	if(header.cmd > 16 || !(validcmds[role] & (1<<header.cmd))) 
	{
		printf("decoder, Invalid Command\n");
		return -1;
	}

	if((m & validsections[header.cmd][0]) != validsections[header.cmd][0]) 
	{
		printf( "decoder, Missing Section\n");
		return -1;
	}

	if((m & ~validsections[header.cmd][1]) != 0) 
	{
		printf("m[%x] valid[%x]", m, validsections[header.cmd][1]);
		return -1;
	}

	return 0;
}



int CTask::DecodeFieldValue(char *d, int l, int mode)
{
	CBinary bin = { l, d };
	if(!bin) return -EC_BAD_SECTION_LENGTH;

	const int num = *bin++;
	if(num==0) return 0;

	/* conditionInfo&updateInfo at largest size in pool, numFields indicate real field */
	CFieldValue *fv;
	if(mode == 0 && conditionInfo)
	{
		fv = conditionInfo;
	}else if(mode != 0 && updateInfo)
	{
		fv = updateInfo;
	}else
	{
		try { fv = new CFieldValue(num); }catch(int err) { return err; }
		if(mode==0)
			conditionInfo = fv;
		else
			updateInfo = fv;
	}

	if(fv->MaxFields() < num)
	{
		fv->Realloc(num);
	}

	int err;
	int keymask = 0;
	for(int i=0; i<num; i++) 
	{
		uint8_t id;
		CValue val;
		uint8_t op;
		uint8_t vt;

		if(!bin)  {
			err = -EC_BAD_SECTION_LENGTH;
			goto bad;
		}

		op = *bin++;
		vt = op & 0xF;
		op >>= 4;

		int nd = 0;
		/* buf -> local -> id */
		err = DecodeFieldId(bin, id, TableDefinition(), nd);
		if(err != 0) goto bad;
		err = -EC_BAD_INVALID_FIELD;
		if(id > NumFields())
			goto bad;

		const int ft = FieldType(id);

		if(vt >= DField::TotalType){
			err = -EC_BAD_VALUE_TYPE;
			goto bad;
		}

		err = -EC_BAD_OPERATOR;

		if(mode==0) {
			if(op >= DField::TotalComparison || validcomps[ft][op]==0)
				goto bad;
			if(id < KeyFields()) {
				if(op != DField::EQ) {
					err = -EC_BAD_MULTIKEY;
					goto bad;
				}
			} else
				ClearAllRows();

			if(validxtype[DField::Set][ft][vt]==0) goto bad;

		} else if(mode==1) {
			if(op != DField::Set) goto bad;
			if(validxtype[op][ft][vt]==0) goto bad;
		} else {
			if(TableDefinition()->IsReadOnly(id)) {
				err = -EC_READONLY_FIELD;
				goto bad;
			}
			if(op >= DField::TotalOperation || validxtype[op][ft][vt]==0)
				goto bad;
		}


		/* avoid one more copy of CValue*/
		/* int(len, value):  buf -> local; buf -> local -> tag */
		/* str(len, value):  buf -> local -> tag; buf -> tag */
		//err = DecodeDataValue(bin, val, vt);
		err = DecodeDataValue(bin, *fv->NextFieldValue(), vt);
		if(err != 0) goto bad;
		//fv->AddValue(id, op, vt, val);
		fv->AddValueNoVal(id, op, vt);
		if(nd)
			requestFlags |= DRequest::Flag::NeedTableDefinition;
		if(id < KeyFields()) 
		{
			if((keymask & (1<<id)) != 0) {
				err = -EC_BAD_MULTIKEY;
				goto bad;
			}
			keymask |= 1<<id;
		} else {
			fv->UpdateTypeMask(TableDefinition()->FieldFlags(id));
		}
	}

	if(!!bin) {
		err = -EC_EXTRA_SECTION_DATA;
		goto bad;
	}

	return 0;
bad:
	/* newman: pool */
	/* free when distructed instread free here*/
	//delete fv;
	return err;
}


int CTask::DecodeResultSet(char * d, int l)
{
	uint32_t nrows;
	int num;

	uint8_t mask[32];
	FIELD_ZERO(mask);

	CBinary bin = { l, d };

	int err = DecodeLength(bin, nrows);
	if(err) return err;

	if(!bin)  return -EC_BAD_SECTION_LENGTH;

	num = *bin++;


	/* buf -> id */
	//check duplicate or bad field id
	int haskey = 0;
	if(num > 0) {
		if(bin < num)
			return -EC_BAD_SECTION_LENGTH;

		for(int i=0; i<num; i++) {
			int t = bin[i];
			if(t == 0)
				haskey = 1;
			if(t==255)
				return -EC_BAD_FIELD_ID;
			if(FIELD_ISSET(t, mask))
				return -EC_DUPLICATE_FIELD;
			FIELD_SET(t, mask);
		}
	}

	//	if(!FlagPassThru() && num != NumFields()+haskey ) return -EINVAL;

	/* result's fieldset at largest size */
	/* field ids: buf -> result */
	if(!result)
	{
		try {
			result = new CResultSet((uint8_t *)bin.ptr, 
				num, 
				NumFields() + 1, 
				TableDefinition());
		} catch(int err) { return err; }
	}else
	{
		if(result->FieldSetMaxFields() < NumFields() + 1)
			result->ReallocFieldSet(NumFields() + 1);
		result->Set((uint8_t *)bin.ptr, num);
	}

	bin += num;
	result->SetValueData(nrows, bin);
	return 0;
}



int CTask::DecodeFieldSet(char *d, int l) {
	uint8_t mask[32];
	FIELD_ZERO(mask);

	CBinary bin = { l, d };
	if(!bin) return -EC_BAD_SECTION_LENGTH;

	const int num = *bin++;
	if(num==0) return 0;
	int realnum = 0;
	uint8_t *idtab=new uint8_t(num);

	/* old: buf -> local -> id -> idtab -> task */
	/* new: buf -> local -> idtab -> task */
	for(int i=0; i<num; i++) {
		int nd=0;
		int rv = DecodeFieldId(bin, idtab[realnum], TableDefinition(), nd);
		if(rv != 0) return rv;
		if(FIELD_ISSET(idtab[realnum], mask)) continue;
		FIELD_SET(idtab[realnum], mask);
		realnum ++;
		if(nd)
			requestFlags |= DRequest::Flag::NeedTableDefinition;
	}

	if(!!bin.len)
		return -EC_EXTRA_SECTION_DATA;

	/* allocate max field in field set, first byte indicate real field num */
	if(!fieldList)
	{
		try { fieldList = new CFieldSet(idtab, 
			realnum, 
			NumFields() + 1); }
		catch(int err) {return -ENOMEM;}
	}else
	{
		if(fieldList->MaxFields() < NumFields() + 1)
		{
			fieldList->Realloc(NumFields() + 1);
		}
		if(fieldList->Set(idtab, realnum) < 0)
		{
			printf("fieldList not exist in pool\n");
			return -EC_TASKPOOL;
		}
	}
	delete []idtab;
	return 0;
}

int CResultSet::DecodeRow(void) {
	if(err) return err;
	if(rowno == numRows) return -EC_NO_MORE_DATA;
	for(int i=0; i<NumFields(); i++) {
		const int id = FieldId(i);
		err = DecodeDataValue(curr, row[id], row.FieldType(id));
		if(err) return err;
	}
	rowno++;
	return 0;
}