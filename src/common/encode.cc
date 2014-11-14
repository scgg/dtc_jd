#include <stdint.h>
#include <endian.h>
#include <byteswap.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include "packet.h"
#include "tabledef.h"
#include "decode.h"
#include "task_base.h"

/* encoding DataValue by type */
char *EncodeDataValue(char *p, const CValue *v, int type) {
	char *t;
	int n;

	switch(type) {
	case DField::None:
		*p++ = 0;
		break;

	case DField::Signed:
	case DField::Unsigned:
	    if(v->s64 >= 0) {
		if(v->s64 < 0x80LL) n = 1;
		else if(v->s64 < 0x8000LL) n = 2;
		else if(v->s64 < 0x800000LL) n = 3;
		else if(v->s64 < 0x80000000LL) n = 4;
		else if(v->s64 < 0x8000000000LL) n = 5;
		else if(v->s64 < 0x800000000000LL) n = 6;
		else if(v->s64 < 0x80000000000000LL) n = 7;
		else n = 8;
	    } else {
		if(v->s64 >= -0x80LL) n = 1;
		else if(v->s64 >= -0x8000LL) n = 2;
		else if(v->s64 >= -0x800000LL) n = 3;
		else if(v->s64 >= -0x80000000LL) n = 4;
		else if(v->s64 >= -0x8000000000LL) n = 5;
		else if(v->s64 >= -0x800000000000LL) n = 6;
		else if(v->s64 >= -0x80000000000000LL) n = 7;
		else n = 8;
	    }
	    t = (char *)&v->s64;
	    *p++ = n;
#if __BYTE_ORDER == __BIG_ENDIAN
	    for(int i=0; i<n; i++)
		p[i] = t[7-n+i];
#else
	    for(int i=0; i<n; i++)
		p[i] = t[n-1-i];
#endif
	    p += n;
	    break;

	case DField::Float:
	    char buf[sizeof(long double)*2+8];
	    if(snprintf(buf, sizeof(buf), __FLTFMT__, (long double)v->flt) >= (int)sizeof(buf))
		memcpy(buf, "NAN", 4);
	    n = strlen(buf)+1;
	    *p++ = n;
	    memcpy(p, buf, n);
	    p += n;
	    break;

	case DField::String:
	case DField::Binary:
	    if(v->str.ptr==NULL) {
		*p++ = 0;
	    } else {
		p = EncodeLength(p, v->str.len+1);
		if(v->bin.len) memcpy(p, v->bin.ptr, v->str.len);
		p += v->str.len;
		*p++ = '\0';
	    }
	    break;
	}
	return p;
}

int EncodedBytesDataValue(const CValue *v, int type) {
	switch(type) {
	case DField::None:
		return 1;
	case DField::Signed:
	case DField::Unsigned:
	    if(v->s64 >= 0) {
		if(v->s64 < 0x80LL) return 2;
		if(v->s64 < 0x8000LL) return 3;
		if(v->s64 < 0x800000LL) return 4;
		if(v->s64 < 0x80000000LL) return 5;
		if(v->s64 < 0x8000000000LL) return 6;
		if(v->s64 < 0x800000000000LL) return 7;
		if(v->s64 < 0x80000000000000LL) return 8;
	    } else {
		if(v->s64 >= -0x80LL) return 2;
		if(v->s64 >= -0x8000LL) return 3;
		if(v->s64 >= -0x800000LL) return 4;
		if(v->s64 >= -0x80000000LL) return 5;
		if(v->s64 >= -0x8000000000LL) return 6;
		if(v->s64 >= -0x800000000000LL) return 7;
		if(v->s64 >= -0x80000000000000LL) return 8;
	    }
	    return 9;
	case DField::Float:
	    char b[sizeof(long double)*2+8];
	    if(snprintf(b, sizeof(b), __FLTFMT__, (long double)v->flt) >= (int)sizeof(b))
		return 5;
	    return 2+strlen(b);
	case DField::String:
	case DField::Binary:
	    if(v->str.ptr==NULL) return 1;
	    return v->str.len+1+EncodedBytesLength(v->str.len+1);
	}
	return 0;
}

/*
 * Encoding simple section
 * <ID> <LEN> <VAL> <ID> <LEN> <VAL>...
 */
int EncodedBytesSimpleSection(const CSimpleSection & sct, uint8_t kt) {
	int len = 0;
	for(int i=0; i<=sct.MaxTags(); i++) {
	    if(sct.TagPresent(i)==0) continue;
	    const int t = i==0 ? kt : sct.TagType(i);
	    len += 1 + EncodedBytesDataValue(sct.GetTag(i), t);
	}
	return len;
}

char *EncodeSimpleSection(char *p, const CSimpleSection & sct, uint8_t kt) {
	for(int i=0; i<=sct.MaxTags(); i++) {
	    if(sct.TagPresent(i)==0) continue;
	    const int t = i==0 ? kt : sct.TagType(i);
	    *p++ = i;
	    p = EncodeDataValue(p, sct.GetTag(i), t);
	}
	return p;
}


/*
 * FieldSet format:
 * 	<NUM> <ID>...
 * 		NUM: 1 byte, total fields
 * 		ID: 1 byte per fieldid
 */
int EncodedBytesFieldSet(const CFieldSet &fs) {
	if(fs.NumFields()==0) return 0;
	if(fs.FieldPresent(0)){
		return fs.NumFields() + 2;
	}
	return fs.NumFields() + 1;
}

char * EncodeFieldSet(char *p, const CFieldSet &fs) {
	if(fs.NumFields()==0) return p;
	*p++ = fs.NumFields();
	for(int i=0; i<fs.NumFields(); i++){
		*p++ = fs.FieldId(i);
		if(fs.FieldId(i) == 0)
			*p++ = 0;
	}
	return p;
}

/*
 * FieldValue format:
 * 	<NUM> <OPER:TYPE> <ID> <LEN> <VALUE>...
 * 		NUM: 1 byte, total fields
 * 		ID: 1 byte
 */
int EncodedBytesFieldValue(const CFieldValue &fv)
{
	if(fv.NumFields()==0) return 0;
	int len = 1;
	for(int i=0; i<fv.NumFields(); i++)
	{
			//for migrate
			if (fv.FieldId(i) == 0)
					len++;
			len += 2 + EncodedBytesDataValue(fv.FieldValue(i), fv.FieldType(i));
	}
	return len;
}

char * EncodeFieldValue(char *p, const CFieldValue &fv)
{
	if(fv.NumFields()==0) return p;
	*p++ = fv.NumFields();
	for(int i=0; i<fv.NumFields(); i++) {
	    const int n = fv.FieldId(i);
	    const int t = fv.FieldType(i);
	    *p++ = (fv.FieldOperation(i)<<4) + t;
		*p++ = n;
		//for migrate
		if (n == 0)
				*p++ = 0;

	    p = EncodeDataValue(p, fv.FieldValue(i), t);
	}
	return p;
}

int EncodedBytesMultiKey(const CValue *v, const CTableDefinition *tdef)
{
	if(tdef->KeyFields() <= 1) return 0;
	int len = 1;
	for(int i=1; i<tdef->KeyFields(); i++)
	    len += 2 + EncodedBytesDataValue(v+i, tdef->FieldType(i));
	return len;
}

char * EncodeMultiKey(char *p, const CValue *v, const CTableDefinition *tdef)
{
	if(tdef->KeyFields() <= 1) return p;
	*p++ = tdef->KeyFields()-1;
	for(int i=1; i<tdef->KeyFields(); i++) {
	    const int t = tdef->FieldType(i);
	    *p++ = ((DField::Set)<<4) + t;
	    *p++ = i;
	    p = EncodeDataValue(p, v+i, t);
	}
	return p;
}

/*
 * FieldSet format:
 * 	<NUM> <FIELD> <FIELD>...
 * 		NUM: 1 byte, total fields
 * 		FIELD: encoded field ID/name
 */
int EncodedBytesFieldSet(const CFieldSetByName &fs) {
	if(fs.NumFields()==0) return 0;
	int len = 1;
	for(int i=0; i<fs.NumFields(); i++) {
//	    len += fs.FieldId(i)?1:2+fs.FieldNameLength(i);
//		fprintf(stderr, "FIELD-SET: field[%s] id[%d] name-len[%d]\n", fs.FieldName(i), fs.FieldId(i), fs.FieldNameLength(i));
		switch(fs.FieldId(i)){
			case 255:
				len += 2+fs.FieldNameLength(i);
				break;
			case 0:
				len += 2;
				break;
			default:
				len += 1;
		}
	}
	return len;
}

char * EncodeFieldSet(char *p, const CFieldSetByName &fs) {
	if(fs.NumFields()==0) return p;
	*p++ = fs.NumFields();
	for(int i=0; i<fs.NumFields(); i++) {
/*	
	    *p++ = fs.FieldId(i);
	    if(fs.FieldId(i)==0) {
		int n = fs.FieldNameLength(i);
		*p++ = n;
		memcpy(p, fs.FieldName(i), n);
		p += n;
	    }
*/
		switch(fs.FieldId(i)){
			case 0:
				*p++ = 0;
				*p++ = 0;
				break;
				
			default:
				*p++ = fs.FieldId(i);
				break;
				
			case 255:
				*p++ = 0;
				const int n = fs.FieldNameLength(i);
				*p++ = n;
				memcpy(p, fs.FieldName(i), n);
				p += n;
				break;
		}
	}
	return p;
}

/*
 * FieldValue format:
 * 	<NUM> <OPER:TYPE> <FIELD> <LEN> <VALUE>
 * 		NUM: 1 byte, total fields
 * 		FIELD: encoded field ID/name
 */
int EncodedBytesFieldValue(const CFieldValueByName &fv) {
	if(fv.NumFields()==0) return 0;
	int len = 1;
	for(int i=0; i<fv.NumFields(); i++) {
//	    len += (fv.FieldId(i) ? 2 : 3 + fv.FieldNameLength(i)) +
//		EncodedBytesDataValue(fv.FieldValue(i), fv.FieldType(i));
		switch(fv.FieldId(i)){
			case 255:
				len += 3 + fv.FieldNameLength(i);
				break;
			case 0:
				len += 3;
				break;
			default:
				len += 2;
		}
		len += EncodedBytesDataValue(fv.FieldValue(i), fv.FieldType(i));
	}
	return len;
}

char * EncodeFieldValue(char *p, const CFieldValueByName &fv) {
	if(fv.NumFields()==0) return p;
	*p++ = fv.NumFields();
	for(int i=0; i<fv.NumFields(); i++) {
	    int n = fv.FieldNameLength(i);
	    int t = fv.FieldType(i);
	    if(t==DField::Unsigned) t = DField::Signed;
	    *p++ = (fv.FieldOperation(i)<<4) + t;
/*		
	    *p++ = fv.FieldId(i);
	    if(fv.FieldId(i)==0) {
		*p++ = n;
		memcpy(p, fv.FieldName(i), n);
		p += n;
	    }
*/		
		switch(fv.FieldId(i)){
			case 0:
				*p++ = 0;
				*p++ = 0;
				break;
			case 255:
				*p++ = 0;
				*p++ = n;
				memcpy(p, fv.FieldName(i), n);
				p += n;
				break;
			default:
				*p++ = fv.FieldId(i);
		}
		
	    p = EncodeDataValue(p, fv.FieldValue(i), t);
	}
	return p;
}

