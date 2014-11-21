#ifndef __CH_TABLEDEF_H_
#define __CH_TABLEDEF_H_

//#include "protocol.h"
#include "section.h"
#include "value.h"
#include "atomicwindows.h"
//#include "md5.h"
//#include "cache_error.h"
//#include "field_api.h"

struct CFieldDefinition {
public:
	enum {
		FF_READONLY = 1,
		FF_VOLATILE = 2,
		FF_DISCARD = 4,
		FF_DESC = 0x10,
		FF_TIMESTAMP = 0x20,
	};
	
	typedef uint8_t fieldflag_t;
public:
	char *fieldName;
	int fieldSize;       //bytes
	uint8_t fieldType;
	uint8_t nameLen;
	uint8_t offset;
	fieldflag_t flags;

	int boffset;       //field的bits起始偏移
	int bsize;         //bits
	uint16_t next;
};

extern const CSectionDefinition tableAttributeDefinition;

class CTableAttribute : public CSimpleSection {
public:
	enum{
		SingleRow = 0x1,
		AdminTable = 0x2,
	};
	
	CTableAttribute() : CSimpleSection(tableAttributeDefinition) {}
	~CTableAttribute() {}
	int IsSingleRow(void) const { return (int)(GetTag(1)?(GetTag(1)->u64 & SingleRow):0); }
	void SetSingleRow(void) { SetTag(1, GetTag(1)?(GetTag(1)->u64| SingleRow):SingleRow); }
	int IsAdminTab(void) const { return (int)(GetTag(1)?(GetTag(1)->u64 & AdminTable):0); }
	void SetAdminTab(void) { SetTag(1, GetTag(1)?(GetTag(1)->u64| AdminTable):AdminTable); }
	unsigned int KeyFieldCount(void) const { return GetTag(3)?GetTag(3)->u64:0; }
	void SetKeyFieldCount(unsigned int n){ SetTag(3, n); }

//压缩字段用来表示设置compressflag的字段id，传给client端使用。
//该标识占用tag1的高八位，共计一个字节
    int CompressFieldId(void) const{return GetTag(1)?(((GetTag(1)->u64)>>56)&0xFF):-1;}
    void SetCompressFlag(int n)
    {
        uint64_t tmp = (((uint64_t)n&0xFF)<<56);
        SetTag(1,GetTag(1)?(GetTag(1)->u64 &tmp):tmp);
    }
};

class CTableDefinition {
private:
	CFieldDefinition *fieldList;
	uint16_t nameHash[256];
	CTableAttribute attr;
	int maxFields;
	int usedFields;
	int numFields;
	int keyFields;
	CBinary tableName;
	char hash[16];

	// only used by client side
	TAtomicU32 count;

	// client side use this determine local/internal object
	CValue *defaultValue;

	// only used by server side
	CBinary packedTableDefinition;
	CBinary packedFullFieldSet;
	CBinary packedFieldSet; // field[0] not include
	uint8_t *uniqFields;
	int rowSize;       //bits //added by jackda
	uint16_t maxKeySize; // max real key size, maybe 0-256. (256 is valid)
	int16_t hasAutoInc; // the autoinc field id, -1:none 0-254, fieldid
	int16_t hasLastacc;
	int16_t hasLastmod;
	int16_t hasLastcmod;
	int16_t hasCompress;//flag for compress
	uint8_t keyFormat; // 0:varsize, 1-255:fixed, large than 255 is invalid
	uint8_t hasDiscard;
	int8_t indexFields; // TREE_DATA, disabled in this release
	uint8_t uniqFieldCnt; //the size of uniqFields member
	uint8_t keysAsUniqField; /* 0 == NO, 1 == EXACT, 2 == SUBSET */

	// no copy
	CTableDefinition(const CTableDefinition &);
public:
	CTableDefinition(int m=0);
	~CTableDefinition(void);

	// client side only
	int Unpack(const char *, int);
	int INC(void) { if(defaultValue==0) return ++count; else return 2; };
	int DEC(void) { if(defaultValue==0) return --count; else return 1; };

	// common methods, by both side
	const char *TableName(void) const { return tableName.ptr; }
	const char *TableHash(void) const { return hash; }
	int IsSameTable(const CBinary &n) const { return StringEqual(tableName, n); }
	int IsSameTable(const char *n) const { return StringEqual(tableName, n); }
	int IsSameTable(const CTableDefinition &r) const { return !memcmp(hash, r.hash, sizeof(hash)); }
	int IsSameTable(const CTableDefinition *r) const { return r ? IsSameTable(*r) : 0; }
	int HashEqual(const CBinary &n) const { return n.len==16 && !memcmp(hash, n.ptr, 16); }
	int NumFields(void) const { return numFields; }
	int KeyFields(void) const { return keyFields; }
	int FieldId(const char *, int=0) const;
	const char *FieldName(int id) const { return fieldList[id].fieldName; }
	int FieldType(int id) const { return fieldList[id].fieldType; }
	int FieldSize(int id) const { return fieldList[id].fieldSize; }
    int MaxFieldSize(void) const 
    {
        int max = 0;
        for (int i = KeyFields(); i<=NumFields(); ++i)
        {
            if (FieldSize(i)>max) {max = FieldSize(i);}
        }
        return max;
    }
	unsigned int FieldFlags(int id) const { return fieldList[id].flags; }
	const char *KeyName(void) const { return fieldList[0].fieldName; }
	int KeyType(void) const { return fieldList[0].fieldType; }
	int KeySize(void) const { return fieldList[0].fieldSize; }
	int IsReadOnly(int n) const { return fieldList[n].flags & CFieldDefinition::FF_READONLY; }
	int IsVolatile(int n) const { return fieldList[n].flags & CFieldDefinition::FF_VOLATILE; }
	int IsDiscard(int n) const { return fieldList[n].flags & CFieldDefinition::FF_DISCARD; }
	int IsDescOrder(int n) const { return fieldList[n].flags & CFieldDefinition::FF_DESC; }
	int IsTimestamp(int n) const { return fieldList[n].flags & CFieldDefinition::FF_TIMESTAMP; }

	int IsSingleRow(void) const { return attr.IsSingleRow(); }
	int IsAdminTable(void) const { return attr.IsAdminTab(); }
//	int IsAdminTable(void) const { return (strcmp(tableName.ptr, "@HOT_BACKUP") == 0); }
	void SetAdminTable(void) { return attr.SetAdminTab(); }
    int CompressFieldId(void) const {return attr.CompressFieldId();}

#if !CLIENTAPI
	// this macro test is scope-test only, didn't affected the class implementation

	// server side only
	//added by jackda 
	//to support Bitmap svr
	const int RowSize(void)const{return (rowSize + 7) / 8;}; //返回rowsize 单位bytes
	const int BRowSize(void)const{return rowSize;};         //返回rowsize 单位bits
	void SetRowSize(int size){rowSize = size;};
	int FieldBSize(int id) const {return fieldList[id].bsize;}
	int FieldBOffset(int id) const {return fieldList[id].boffset;}
	int FieldOffset(int id) const { return fieldList[id].offset; }
	
	void Pack(CBinary &v) { v = packedTableDefinition; }
	int BuildInfoCache(void);
	const CBinary &PackedFieldSet(void) const { return packedFullFieldSet; }
	const CBinary &PackedFieldSet(int withKey) const { return withKey?packedFullFieldSet:packedFieldSet;}
	const CBinary &PackedDefinition(void) const { return packedTableDefinition; }

	int SetKey(const char *name, uint8_t type, int size) { return AddField(0, name, type, size); }
	void SetSingleRow(void) { attr.SetSingleRow(); };
	int SetTableName(const char *);
	void SetDefaultValue(int id, const CValue *val);
	void GetDefaultRow(CValue *) const;
	const CValue *DefaultValue(int id) const { return &defaultValue[id]; }
	void SetAutoIncrement(int n) { if(n>=0 && n <= numFields) hasAutoInc = n; }
	void SetLastacc(int n) { if(n>=0 && n <= numFields) hasLastacc = n; fieldList[n].flags |= CFieldDefinition::FF_TIMESTAMP; }
	void SetLastmod(int n) { if(n>=0 && n <= numFields) hasLastmod = n; fieldList[n].flags |= CFieldDefinition::FF_TIMESTAMP; }
	void SetLastcmod(int n) { if(n>=0 && n <= numFields) hasLastcmod = n; fieldList[n].flags |= CFieldDefinition::FF_TIMESTAMP; }
	void SetCompressFlag(int n) { if(n>=0 && n <= numFields) attr.SetCompressFlag(n);}
	int HasDiscard(void) const { return hasDiscard; }
	int HasAutoIncrement(void) const { return hasAutoInc >= 0; }
	int KeyAutoIncrement(void) const { return hasAutoInc == 0; }
	int AutoIncrementFieldId(void) const { return hasAutoInc; }
	int LastaccFieldId(void) const { return hasLastacc; }
	int LastmodFieldId(void) const { return hasLastmod; }
	int LastcmodFieldId(void) const { return hasLastcmod; }
	void MarkAsReadOnly(int n) { fieldList[n].flags |= CFieldDefinition::FF_READONLY; }
	void MarkAsVolatile(int n) { fieldList[n].flags |= CFieldDefinition::FF_VOLATILE; }
	void MarkAsDiscard(int n) { fieldList[n].flags |= CFieldDefinition::FF_DISCARD; hasDiscard=1; }
	void MarkOrderDesc(int n){ fieldList[n].flags |= CFieldDefinition::FF_DESC; }
	void MarkOrderAsc(int n){ fieldList[n].flags &= ~CFieldDefinition::FF_DESC; }
	uint8_t UniqFields() const { return uniqFieldCnt; }
	uint8_t *UniqFieldsList(){   return uniqFields;}
	const uint8_t *UniqFieldsList() const {   return uniqFields;}
	void MarkUniqField(int n){ uniqFields[uniqFieldCnt++] = n; }
	int KeyAsUniqField() const { return keysAsUniqField==1?true:false; }
	int KeyPartOfUniqField() const { return keysAsUniqField; } /* 0 -- NOT, 1 -- EXACT, 2 -- SUBSET */
	int IndexFields() const { return indexFields; }
	void SetIndexFields(int n){ indexFields = n; }

	int SetKeyFields(int n=1);
	int KeyFormat(void) const { return keyFormat; }
	int MaxKeySize(void) const { return maxKeySize; }

	CValue PackedKey(const char *pk){
		CValue key;
		key.bin.ptr =(char *) pk;
		key.bin.len = KeyFormat()?0:(*(unsigned char *)pk+1);
		return key;
	}

	int AddField(int id, const char *name, uint8_t type, int size);
	int AddField(int id, const char *name, uint8_t type, int size,int bsize,int boffset);
#endif
};

class CTableReference {
	private:
		CTableDefinition *tableDef;
	public:
		CTableDefinition *TableDefinition(void) const { return tableDef; }
		operator CTableDefinition *(void) { return tableDef; }
		CTableReference(CTableDefinition *t=0) { tableDef = t; }
		CTableReference(const CTableReference &c) { tableDef = c.TableDefinition(); }
		virtual ~CTableReference(void) {}

		inline void SetTableDefinition(CTableDefinition *t) { tableDef = t; }

		int NumFields(void) const { return tableDef->NumFields(); }
		int KeyFields(void) const { return tableDef->KeyFields(); }
		int KeyType(void) const { return tableDef->KeyType(); }
		const char *KeyName(void) const { return tableDef->KeyName(); }
		int KeySize(void) const { return tableDef->KeySize(); }
		int FieldType(int n) const { return tableDef->FieldType(n); }
		int FieldSize(int n) const { return tableDef->FieldSize(n); }
		int FieldId(const char *n) const { return tableDef->FieldId(n); }
		const char* FieldName(int id) const { return tableDef->FieldName(id); }
		int IsSameTable(const CBinary &n) const { return tableDef->IsSameTable(n); }
		int IsSameTable(const char *n) const { return tableDef->IsSameTable(n); }
		int IsSameTable(const CTableDefinition &r) const { return tableDef->IsSameTable(r); }
		int IsSameTable(const CTableDefinition *r) const { return tableDef->IsSameTable(r); }
		int HashEqual(const CBinary &n) const { return tableDef->HashEqual(n); }
		const char *TableName(void) const { return tableDef->TableName(); }
		const char *TableHash(void) const { return tableDef->TableHash(); }

#if !CLIENTAPI
		// this macro test is scope-test only, didn't affected the class implementation
		int KeyFormat(void) const { return tableDef->KeyFormat(); }
		int KeyAutoIncrement(void) const { return tableDef->KeyAutoIncrement(); }
		int AutoIncrementFieldId(void) const { return tableDef->AutoIncrementFieldId(); }
		int FieldOffset(int n) const { return tableDef->FieldOffset(n); }
		int FieldBSize(int n) const {return tableDef->FieldBSize(n);}
		int FieldBOffset(int n) const {return tableDef->FieldBOffset(n);}
		void GetDefaultRow(CValue *value) const { tableDef->GetDefaultRow(value); }
#endif
};








#endif
