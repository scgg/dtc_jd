#include "result.h"
#include "decode.h"

CResultWriter::CResultWriter(const CFieldSet *fs, unsigned int st, unsigned int cnt) {
	if(cnt==0) st = 0;
	limitStart = st; limitNext = st+cnt;
	const int nf = fs==NULL ? 0 : fs->NumFields();
	fieldSet = nf ? fs : NULL;
	totalRows = 0;
	numRows = 0;
}

int CResultWriter::SetRows(unsigned int rows) {
	if(fieldSet==NULL /* CountOnly */ ||
			(numRows==0 && rows <= limitStart /* Not in Range */))
	{
		totalRows = rows;
		numRows = limitNext==0 ? rows :
			rows <= limitStart ? 0 :
			rows <= limitNext ? rows - limitStart :
			limitNext - limitStart;
	}
	else if(IsFull()==0)
		return -1;
	else
		totalRows = rows;
	
	return 0;
}

CResultPacket::CResultPacket(const CFieldSet *fs, unsigned int st, unsigned int cnt) :
	CResultWriter(fs, st, cnt)
{
	const int nf = fs==NULL ? 0 : fs->NumFields();
	int len = 5+1+nf;
	//bc = (CBufferChain *)MALLOC(sizeof(CBufferChain) + len);
	bc = (CBufferChain *)MALLOC(1024);
	if(bc==NULL) throw (int)-ENOMEM;
	bc->nextBuffer = NULL;
	bc->usedBytes = len;
	//bc->totalBytes = len;
	bc->totalBytes = 1024-sizeof(CBufferChain);
	rowDataBegin = bc->usedBytes;
	char *p = bc->data + 5;
	*p++ = nf;
	if(nf) {
	    for(int i=0; i<nf; i++)
		*p++ = fs->FieldId(i);
	}
}

/* newman: pool */
void CResultPacket::Clean()
{
    CResultWriter::Clean();
    if(bc)
	bc->Clean();
}

/* newman: pool, bc should exist */
int CResultPacket::Set(const CFieldSet * fs, unsigned int st, unsigned int ct)
{
    CResultWriter::Set(fs,st, ct);
    const int nf = fs==NULL ? 0 : fs->NumFields();
    int len = 5+1+nf;
    if(bc==NULL) return -1;
    bc->nextBuffer = NULL;
    bc->usedBytes = len;
    rowDataBegin = bc->usedBytes;
    char *p = bc->data + 5;
    *p++ = nf;
    if(nf) {
	for(int i=0; i<nf; i++)
	    *p++ = fs->FieldId(i);
    }

    return 0;
}

/* resultPacket is just a buff */
CResultPacket::~CResultPacket(void)
{
	FREE_IF(bc);
}

static int Expand(CBufferChain *&bc, int addsize) {
	int expectsize = bc->usedBytes + addsize;
	if(bc->totalBytes >= expectsize)
		return 0;

	// add header and round to 8 byte aligned
	expectsize += sizeof(CBufferChain);
	expectsize |= 7;
	expectsize += 1;

	int sparsesize = expectsize;
	if(sparsesize > addsize*16)
		sparsesize = addsize * 16;

	sparsesize += expectsize;
	if(REALLOC(bc, sparsesize) != NULL) {
		bc->totalBytes = sparsesize - sizeof(CBufferChain);
		return 0;
	}
	if(REALLOC(bc, expectsize) != NULL) {
		bc->totalBytes = expectsize - sizeof(CBufferChain);
		return 0;
	}
	return -1;
}

int CResultPacket::AppendRow(const CRowValue &r) {
	int ret = 0;
	totalRows++;
	if(limitNext>0) {
		if(totalRows <= limitStart || totalRows > limitNext)
			return 0;
	}
	if(fieldSet) {
	    int len = 0;
	    for(int i=0; i < fieldSet->NumFields(); i++) {
			const int id = fieldSet->FieldId(i);
            len += EncodedBytesDataValue(r.FieldValue(id), r.FieldType(id));
	    }

#if 0
	    CBufferChain *c = (CBufferChain *)REALLOC(bc,
		    sizeof(CBufferChain)+ bc->usedBytes + len);

	    if(c==NULL) return -ENOMEM;
	    bc = c;
#else
	    if(Expand(bc, len) != 0) return -ENOMEM;
#endif

	    char *p = bc->data + bc->usedBytes;
	    bc->usedBytes += len;
        /* totalBytse not controled here */
	    //bc->totalBytes = bc->usedBytes;

	    for(int i=0; i < fieldSet->NumFields(); i++) {
		const int id = fieldSet->FieldId(i);
		p = EncodeDataValue(p, r.FieldValue(id), r.FieldType(id));
	    }
	    ret = 1;
	}
	numRows = totalRows - limitStart;
	return ret;
}

int CResultPacket::MergeNoLimit(const CResultWriter *rp0)
{
    const CResultPacket &rp = *(const CResultPacket *)rp0;

    int expectedsize = bc->usedBytes + (rp.bc->usedBytes-rp.rowDataBegin);
    if(bc->totalBytes < expectedsize)
    {
	CBufferChain *c = (CBufferChain *)REALLOC(bc, sizeof(CBufferChain)+ expectedsize + 1024);
	if(c==NULL) return -ENOMEM;
	bc = c;
	bc->totalBytes = expectedsize + 1024;
    }

    char *p = bc->data + bc->usedBytes;
    memcpy(p, rp.bc->data+rp.rowDataBegin, rp.bc->usedBytes - rp.rowDataBegin);

    bc->usedBytes += rp.bc->usedBytes - rp.rowDataBegin;
    totalRows += rp.totalRows;
    numRows += rp.numRows;

    return 0;
}

