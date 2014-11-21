#ifndef __CH_FIELD_API_H__
#define __CH_FIELD_API_H__

#define INVALID_FIELD_ID	255
class CFieldSetByName {
private:
	uint16_t maxFields;
	uint8_t numFields;
	uint8_t solved;
	uint8_t maxvid;
	struct {
	    uint8_t fid;
	    uint8_t vid;
	    uint8_t nlen;
	    char *name;
	} *fieldValue;

	CFieldSetByName(const CFieldSetByName&); // NOT IMPLEMENTED
public:

	CFieldSetByName(void): maxFields(0), numFields(0), solved(1), maxvid(0), fieldValue(NULL) { }
	~CFieldSetByName() {
	    if(fieldValue) {
		for(int i=0; i<numFields; i++)
		    FREE(fieldValue[i].name);
		FREE(fieldValue);
	    }
	}

	int Solved(void) const { return solved; }
	int Resolve(const CTableDefinition *, int);
	void Unresolve(void);

	int MaxFields(void) const { return maxFields; }
	int NumFields(void) const { return numFields; }

	int FieldPresent(const char *name) const {
	    for(int i=0; i<numFields; i++)
		if(strncasecmp(fieldValue[i].name, name, 256)==0)
		    return 1;
	    return 0;
	}

	const char *FieldName(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].name : NULL;
	}

	int FieldNameLength(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].nlen : 255;
	}

	int FieldId(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].fid : 255;
	}

	int VirtualId(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].vid : 0;
	}

	int MaxVirtualId(void) const {
	    return maxvid;
	}

	int AddField(const char *name, int vid);
	int FieldVId(const char* name) const {
	    for(int i=0; i<numFields; i++)
		if(strncasecmp(fieldValue[i].name, name, 256)==0)
		    return fieldValue[i].vid;
	    return -1;
	}
	
	const uint8_t *VirtualMap(void) const;
};


class CFieldValueByName {
private:
	uint16_t maxFields;
	uint8_t numFields;
	uint8_t solved;

	struct {
	    uint8_t type;
	    uint8_t oper;
	    uint8_t fid;
	    uint8_t nlen;
	    char *name;
	    CValue val;
	} *fieldValue;

	CFieldValueByName(const CFieldValueByName &); // NOT IMPLEMENTED
public:

	CFieldValueByName(void) : maxFields(0), numFields(0), solved(1), fieldValue(NULL) {
	}

	~CFieldValueByName() {
	    if(fieldValue) {
		for(int i=0; i<numFields; i++) {
			FREE_IF(fieldValue[i].name);
		    if (fieldValue[i].type==DField::String ||
			fieldValue[i].type==DField::Binary)
			FREE_IF(fieldValue[i].val.bin.ptr);
		}
		FREE(fieldValue);
	    }
	}

	int Solved(void) const { return solved; }
	int Resolve(const CTableDefinition *, int);
	void Unresolve(void);

	int MaxFields(void) const { return maxFields; }
	int NumFields(void) const { return numFields; }

	const char * FieldName(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].name : NULL;
	}

	int FieldNameLength(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].nlen : 0;
	}

	int FieldId(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].fid : 255;
	}

	int FieldType(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].type : DField::None;
	}

	int FieldOperation(int n) const {
	    return n>=0 && n<numFields ? fieldValue[n].oper : 0;
	}

	const CValue *FieldValue(int n) const {
	    return n>=0 && n<numFields ? &fieldValue[n].val : NULL;
	}

	int AddValue(const char *n, uint8_t op, uint8_t t, const CValue &v);
};

#endif
