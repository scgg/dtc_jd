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
#include <assert.h>
#include <new>

#include "value.h"
//#include "tabledef.h"
#include "protocol.h"

class CTableDefinition;
class CFieldValue;

class CFieldSet {
private:
	uint8_t *fieldId;
	uint8_t fieldMask[32];
	
public:

	CFieldSet(const CFieldSet &fs) {
		int n = fs.fieldId[-2];
		fieldId = (uint8_t *)malloc(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		memcpy(fieldId, fs.fieldId-2, n+2);
		fieldId += 2;
	}
	CFieldSet(int n) {
		if(n > 255) n = 255;
		fieldId = (uint8_t*)malloc(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		fieldId += 2;
		fieldId[-2] = n;
		memset(fieldId-1, 0, n+1);
		memset(fieldMask, 0, sizeof(fieldMask));
	}
	CFieldSet(uint8_t *idtab, int n) {
		if(n > 255) n = 255;
		fieldId = (uint8_t*)malloc(n+2);
		if(fieldId==NULL) throw std::bad_alloc();
		*fieldId++ = n;
		*fieldId++ = n;
		memcpy(fieldId, idtab, n);
		BuildFieldMask(fieldMask);
	}
	~CFieldSet() {
		if(fieldId) free(fieldId-2);
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
	void BuildFieldMask(uint8_t *mask) const;
};

class CTableReference {
    public:
        CTableReference(void *t) : ref(t) {}
        void *TableDefinition(void) const { return ref; }
        void *ref;
        int NumFields() const { return 255; }
};

class CRowValue: public CTableReference {
private:
	CValue *value;
public:

	CRowValue(CTableDefinition *t) : CTableReference(t)
	{
		assert(TableDefinition()!=NULL);
		assert(NumFields()>0);
		value = (CValue *)calloc(NumFields()+1, sizeof(CValue));
		if(value == NULL) throw std::bad_alloc();
	};

	CRowValue(const CRowValue &r): CTableReference(r.TableDefinition()) {
		value = (CValue *)calloc(NumFields()+1, sizeof(CValue));
		if(value == NULL) throw std::bad_alloc();
		memcpy(value, r.value, sizeof(CValue)*NumFields()+1);
	}

	~CRowValue() {
		if(value) free(value);
	};

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
	void CopyValue( const CValue *v, int id, int n) {
		memcpy(&value[id], v, n*sizeof(CValue));
	}
};

class CFieldValue {
private:
	struct {
	    uint8_t id;
	    uint8_t oper;
	    uint8_t type;
	    CValue val;
	} *fieldValue;

	int maxFields;
	int numFields;
	int typeMask;

public:

	CFieldValue(int total) {
	    fieldValue = NULL;
	    maxFields = numFields = 0;
	    if(total <= 0) return;
	    fieldValue = (typeof(fieldValue))malloc(total * sizeof(*fieldValue));
	    if(fieldValue==NULL) throw(-ENOMEM);
	    maxFields = total;
	    typeMask = 0;
	}
	CFieldValue(const CFieldValue &fv, int sparse=0)
	{
		numFields = fv.numFields;
		typeMask = fv.typeMask;
		if(sparse < 0) sparse = 0;
		sparse += fv.numFields;
		maxFields = sparse;
		if(fv.fieldValue != NULL){
			fieldValue = (typeof(fieldValue))malloc(sparse * sizeof(*fieldValue));
			if(fieldValue==NULL) throw(-ENOMEM);
			memcpy(fieldValue, fv.fieldValue, fv.numFields * sizeof(*fieldValue));
		}
		else{
			fieldValue = NULL;
		}
	}
	~CFieldValue() {
	    if(fieldValue) free(fieldValue);
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
	void UpdateTypeMaskRO(int ro) { typeMask |= (ro ? 1 : 2); };
	void UpdateTypeMaskSync(int sy) { typeMask |= (sy ? 4 : 8); };
	int HasTypeRO(void) const { return typeMask & 1; }
	int HasTypeRW(void) const { return typeMask & 2; }
	int HasTypeSync(void) const { return typeMask & 4; }
	int HasTypeAsync(void) const { return typeMask & 8; }

	void BuildFieldMask(uint8_t *mask) const;
	int Update(CRowValue &);
	int Compare(const CRowValue &, int iCmpFirstNRows=256);
};

#endif
