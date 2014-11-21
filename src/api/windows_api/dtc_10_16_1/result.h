#ifndef __CH_RESUL_H__
#define __CH_RESUL_H__
#include "field.h"

class CResultSet: public CFieldSet {
private:
	int err;
	CBinary init;
	CBinary curr;
	int numRows;
	int rowno;
	CRowValue row;

public:
	/*fieldset at largest size*/
	CResultSet(const uint8_t *idtab, int num, int total, CTableDefinition *t) :
	    CFieldSet(idtab, num, total),
	    err(0),
	    init(CBinary::Make(NULL,0)),
	    curr(CBinary::Make(NULL,0)),
	    numRows(0),
	    rowno(0),
	    row(t)
	{
	}

	CResultSet(const CFieldSet&f, CTableDefinition *t) :
	    CFieldSet(f),
	    err(0),
	    init(CBinary::Make(NULL,0)),
	    curr(CBinary::Make(NULL,0)),
	    numRows(0),
	    rowno(0),
	    row(t)
	{
	}

	~CResultSet(void) {
	}

	inline void Clean()
	{
	    CFieldSet::Clean();
	    err = 0;
	    init = CBinary::Make(NULL,0);
	    curr = CBinary::Make(NULL,0);
	    numRows = 0;
	    rowno = 0; 
        row.Clean();
	}

	inline int FieldSetMaxFields()
	{
		return CFieldSet::MaxFields();
	}

	inline void ReallocFieldSet(int total)
	{
		CFieldSet::Realloc(total);                     
	}

	/* clean before set*/
	inline void Set(const uint8_t *idtab, int num)
	{
	    CFieldSet::Set(idtab, num); 
	    //row.SetTableDefinition(t);
	}

	inline void SetValueData(int nr, CBinary b) {
		rowno = 0;
		numRows = nr;
		init = b;
		curr = b;
	}

	int TotalRows(void) const {
		return numRows;
	}

	const CValue &operator [](int id) const { return row[id]; }
	const CValue *FieldValue(int id) const {
		return row.FieldValue(id);
	}
	const CValue *FieldValue(const char *id) const {
		return row.FieldValue(id);
	}

	int DecodeRow(void);

	const CRowValue *RowValue(void) const {
	    if(err) return NULL;
	    return &row;
	}

	const CRowValue *FetchRow(void) {
	    if(DecodeRow()) return NULL;
	    return &row;
	}

	CRowValue *__FetchRow(void) {
	    if(DecodeRow()) return NULL;
	    return &row;
	}

	int ErrorNum(void) const { return err; }

	int Rewind(void) { curr = init; err = 0; rowno = 0; return 0; }

	int DataLen(void) const { return init.len; }
};

class CResultWriter {
	CResultWriter(const CResultWriter &); // NOT IMPLEMENTED
public:
	const CFieldSet *fieldSet;
	unsigned int limitStart, limitNext;
	unsigned int totalRows;
	unsigned int numRows;
	
	CResultWriter(const CFieldSet *, unsigned int, unsigned int);
	virtual ~CResultWriter(void) { }

	inline virtual void Clean()
	{
	    totalRows = 0;
	    numRows = 0;
	}
	virtual void DetachResult() = 0;
	inline virtual int Set(const CFieldSet * fs, unsigned int st, unsigned int cnt)
	{ 
	    if(cnt==0) st = 0;
	    limitStart = st; limitNext = st+cnt;
	    const int nf = fs==NULL ? 0 : fs->NumFields();
	    fieldSet = nf ? fs : NULL;

	    return 0;
	}
	virtual int AppendRow(const CRowValue &) = 0;
	virtual int MergeNoLimit(const CResultWriter* rp) = 0;

	int SetRows(unsigned int rows);
	void SetTotalRows(unsigned int totalrows){ totalRows = totalrows; }
	void AddTotalRows(int n) { totalRows += n; }
	int IsFull(void) const { return limitNext > 0 && totalRows >= limitNext; }
	int InRange(unsigned int total, unsigned int begin=0) const { 
		if(limitNext==0)
			return(1);
		if(total <= limitStart || begin >= limitNext)
			return(0);
		return(1);
	}
};

class CResultPacket: public CResultWriter {
	CResultPacket(const CResultPacket &); // NOT IMPLEMENTED
public:
    /* acctually just one buff */
	CBufferChain *bc;
	unsigned int rowDataBegin;
	
	CResultPacket(const CFieldSet *, unsigned int, unsigned int);
	virtual ~CResultPacket(void);

	void Clean();
	inline virtual void DetachResult() { bc = NULL; }
	virtual int Set(const CFieldSet * fieldList, unsigned int st, unsigned int ct);
	virtual int AppendRow(const CRowValue &);
	virtual int MergeNoLimit(const CResultWriter* rp);
};

class CResultBuffer: public CResultWriter {
	CResultBuffer(const CResultBuffer &); // NOT IMPLEMENTED
public:
	CResultBuffer(const CFieldSet *, unsigned int, unsigned int);
	virtual ~CResultBuffer(void);

	virtual int AppendRow(const CRowValue &);
	virtual int MergeNoLimit(const CResultWriter* rp);
};

#endif
