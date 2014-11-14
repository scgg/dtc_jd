#ifndef __CH_FIELD_H__
#define __CH_FIELD_H__

/*
 * field: column value of table
 * FieldSet: set of field
 * FieldValue: set of combo(field, oper, type, value)
 * 	oper can't be operation or comparison
 * ...ByName: used by client API, field can be ID or name
 * RowValue: a row of value, same element with total subtable fields
 * ResultSet: some RowValue, subtable result received from server/helper
 * ResultPacket: encoded subtable result, prepare for sending
 */
#include <string.h>
#include <errno.h>

#include "value.h"
#include "tabledef.h"
#include "protocol.h"
#include "buffer.h"

class CFieldValue;
class CFieldSetByName;
class CFieldValueByName;

class CFieldSet {
private:
	uint8_t *fieldId;
	uint8_t fieldMask[32];
	
public:

	CFieldSet(const CFieldSet &fs) {
		int n = fs.fieldId[-2];
		fieldId = (uint8_t *)MALLOC(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		memcpy(fieldId, fs.fieldId-2, n+2);
		memcpy(fieldMask, fs.fieldMask, sizeof(fieldMask));
		fieldId += 2;
	}
	CFieldSet(int n) {
		if(n > 255) n = 255;
		fieldId = (uint8_t*)MALLOC(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		fieldId += 2;
		fieldId[-2] = n;
		memset(fieldId-1, 0, n+1);
		memset(fieldMask, 0, sizeof(fieldMask));
	}
	CFieldSet(const uint8_t *idtab, int n) {
		if(n > 255) n = 255;
		fieldId = (uint8_t*)MALLOC(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		*fieldId++ = n;
		*fieldId++ = n;
		memcpy(fieldId, idtab, n);
		BuildFieldMask(fieldMask);
	}

	/* allocate max field cnt, real field num in first and second byte */
	CFieldSet(const uint8_t *idtab, int n, int total) {
		if(n > 255) n = 255;
		if(total > 255) total = 255;
		fieldId = (uint8_t*)MALLOC(total+2);
		if(fieldId==NULL) throw std::bad_alloc();
		*fieldId++ = total;
		*fieldId++ = n;
		memcpy(fieldId, idtab, n);
		memset(fieldMask, 0, sizeof(fieldMask));
		BuildFieldMask(fieldMask);
	}

	inline void Clean()
	{
	    fieldId[-1] = 0;
	    memset(fieldMask, 0, sizeof(fieldMask));
	}

	/* clean before set */
	inline int Set(const uint8_t *idtab, int n)
	{
	    if(n > 255) n = 255;
	    if(fieldId==NULL)
		return -1;
	    fieldId[-1] = n;
	    memcpy(fieldId, idtab, n);
	    BuildFieldMask(fieldMask);
	    return 0;
	}

	inline int MaxFields() { return fieldId[-2]; }
	inline void Realloc(int total)
	{
		fieldId -= 2;
		fieldId = (uint8_t*)realloc(fieldId, total + 2);
		*fieldId = total;
		fieldId += 2;
	}

	int Copy(const CFieldSetByName &);
	~CFieldSet() {
		if(fieldId) FREE(fieldId-2);
	}

	int NumFields(void) const { return fieldId[-1]; }
	int FieldId(int n) const {
	    return n>=0 && n<NumFields() ? fieldId[n] : 0;
	}
	void AddField(int id) {
		const int n = fieldId[-1];
		if(n >= fieldId[-2]) return;
		fieldId[n] = id;
		fieldId[-1]++;
		FIELD_SET(id, fieldMask);
	}
	int FieldPresent(int id) const {
		if(id >=0 && id<255)
			return FIELD_ISSET(id, fieldMask);
		return 0;
	}
	void BuildFieldMask(uint8_t *mask) const {
		for(int i=0; i<NumFields(); i++)
			FIELD_SET(fieldId[i], mask);
	}

};

class CRowValue: public CTableReference {
private:
	CValue *value;
public:

	CRowValue(CTableDefinition *t) : CTableReference(t)
	{
		value = (CValue *)calloc(NumFields()+1, sizeof(CValue));
		if(value == NULL) throw std::bad_alloc();
	};

	CRowValue(const CRowValue &r): CTableReference(r.TableDefinition()) {
		value = (CValue *)calloc(NumFields()+1, sizeof(CValue));
		if(value == NULL) throw std::bad_alloc();
		memcpy(value, r.value, sizeof(CValue)*(NumFields()+1));
	}

	virtual ~CRowValue() {
		FREE_IF(value);
	};

    inline void Clean()
    {
        memset(value, 0, sizeof(CValue) * (NumFields()+1));
    }

#if 0
	int KeyFields(void) const { return tableDef->KeyFields(); }
        int NumFields(void) const { return tableDef->NumFields(); }
        int FieldType(int n) const { return tableDef->FieldType(n); }
	int FieldSize(int n) const { return tableDef->FieldSize(n); }
	int FieldBSize(int n) const {return tableDef->FieldBSize(n);}
	int FieldBOffset(int n) const {return tableDef->FieldBOffset(n);}
	int IsSameTable(const CRowValue &rv) const { return tableDef->IsSameTable(rv.tableDef); }
	int IsSameTable(const CRowValue *rv) const { return rv ? IsSameTable(*rv) : 0; }
	const CTableDefinition *TableDefinition(void) const { return tableDef; }
#endif
#if !CLIENTAPI
	// this macro test is scope-test only, didn't affected the class implementation
	void DefaultValue(void) { GetDefaultRow(value); }
#endif
	int IsSameTable(const CRowValue &rv) const { return IsSameTable(rv.TableDefinition()); }
	int IsSameTable(const CRowValue *rv) const { return rv ? IsSameTable(rv->TableDefinition()) : 0; }

	/*Compare tow CRowValue by FieldIDList*/
	int Compare(const CRowValue &rv, uint8_t *fieldIDList, uint8_t num);
	CValue & operator [] (int n) { return value[n]; }
	const CValue & operator [] (int n) const { return value[n]; }
	CValue *FieldValue(int id) {
	    return id>=0 && id<=NumFields() ? &value[id] : NULL;
	}
	const CValue *FieldValue(int id) const {
	    return id>=0 && id<=NumFields() ? &value[id] : NULL;
	}
	CValue *FieldValue(const char *name) {
		return FieldValue(FieldId(name));
	}
	const CValue *FieldValue(const char *name) const {
		return FieldValue(FieldId(name));
	}
	void CopyValue( const CValue *v, int id, int n) {
		memcpy(&value[id], v, n*sizeof(CValue));
	}
	void UpdateTimestamp(uint32_t now, int updateall/*update all timestamp, include lastcmod*/);
};

class CFieldValue {
private:
	struct SFieldValue{
	    uint8_t id;
	    uint8_t oper;
	    uint8_t type;
	    CValue val;
	} *fieldValue;

	//total
	int maxFields;
	//real
	int numFields;
	CFieldDefinition::fieldflag_t typeMask[2];

public:

	CFieldValue(int total) {
	    fieldValue = NULL;
	    maxFields = numFields = 0;
	    if(total <= 0) return;
	    fieldValue = (SFieldValue*)MALLOC(total * sizeof(*fieldValue));
	    if(fieldValue==NULL) throw(-ENOMEM);
	    maxFields = total;
	    typeMask[0] = 0;
	    typeMask[1] = 0;
	}
	CFieldValue(const CFieldValue &fv, int sparse=0)
	{
		numFields = fv.numFields;
		typeMask[0] = fv.typeMask[0];
		typeMask[1] = fv.typeMask[1];
		if(sparse < 0) sparse = 0;
		sparse += fv.numFields;
		maxFields = sparse;
		if(fv.fieldValue != NULL){
			fieldValue = (SFieldValue*)MALLOC(sparse * sizeof(*fieldValue));
			if(fieldValue==NULL) throw(-ENOMEM);
			memcpy(fieldValue, fv.fieldValue, fv.numFields * sizeof(*fieldValue));
		}
		else{
			fieldValue = NULL;
		}
	}
	int Copy(const CFieldValueByName &, int mode, const CTableDefinition *);
	~CFieldValue() {
	    FREE_IF(fieldValue);
	}

	/* should be inited as just constructed */
	inline void Clean()
	{
	    numFields = 0;
	    memset(typeMask, 0, 2 * sizeof(CFieldDefinition::fieldflag_t));
	}

	inline void Realloc(int total)
	{
		maxFields = total;
		fieldValue = (struct SFieldValue *)realloc(fieldValue, sizeof(struct SFieldValue) * total);
	}

	int MaxFields(void) const { return maxFields; }
	int NumFields(void) const { return numFields; }

	int FieldId(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].id : 0;
	}

	int FieldType(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].type : DField::None;
	}

	int FieldOperation(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].oper : 0;
	}

	CValue *FieldValue(int n) const {
	    return n>=0 && n<numFields ? &fieldValue[n].val : NULL;
	}

	void AddValue(uint8_t id, uint8_t op, uint8_t t, const CValue &val) {
		if(numFields==maxFields) return;
		fieldValue[numFields].id = id;
		fieldValue[numFields].oper = op;
		fieldValue[numFields].type = t;
		fieldValue[numFields].val = val;
		numFields++;
	}
	CValue * NextFieldValue() {if(numFields==maxFields) return NULL; return &fieldValue[numFields].val;}
	void AddValueNoVal(uint8_t id, uint8_t op, uint8_t t) {
	    if(numFields==maxFields) return;
	    fieldValue[numFields].id = id;
	    fieldValue[numFields].oper = op;
	    fieldValue[numFields].type = t;
	    numFields++;
	}
	void UpdateTypeMask(unsigned int flag) { typeMask[0] |= flag; typeMask[1] |= ~flag; }
	int HasTypeRO(void) const { return typeMask[0] & CFieldDefinition::FF_READONLY; }
	int HasTypeRW(void) const { return typeMask[1] & CFieldDefinition::FF_READONLY; }
	int HasTypeSync(void) const { return 1; }
	int HasTypeAsync(void) const { return 0; }
	int HasTypeVolatile(void) const { return typeMask[0] & CFieldDefinition::FF_VOLATILE; }
	int HasTypeCommit(void) const { return typeMask[1] & CFieldDefinition::FF_VOLATILE; }
	int HasTypeTimestamp(void) const { return typeMask[0] & CFieldDefinition::FF_TIMESTAMP; }

	void BuildFieldMask(uint8_t *mask) const {
		for(int i=0; i<NumFields(); i++)
			FIELD_SET(fieldValue[i].id, mask);
	}

	int Update(CRowValue &);
	int Compare(const CRowValue &, int iCmpFirstNRows=256);
};

#endif
