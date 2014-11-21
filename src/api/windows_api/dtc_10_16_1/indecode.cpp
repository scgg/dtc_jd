#include "indecode.h"
#include "value.h"

#define __FLTFMT__	"%LA"
#ifdef _POSIX2_RE_DUP_MAX
#define	DUPMAX	_POSIX2_RE_DUP_MAX
#else
#define	DUPMAX	255
#endif
#define	INFINITY	(DUPMAX + 1)
#define	NC		(CHAR_MAX - CHAR_MIN + 1)
/* encoding DataValue by type */

// BYTE0           BYTE1   BYTE2   BYTE3   BYTE4
// 0-1110111						<224
// 11110000-1100   8					<3328
// 11111101        8       8				<64K
// 11111110        8       8       8			<16M
// 11111111        8       8       8       8		<4G
int DecodeLength(CBinary &bin, uint32_t &len) {
	if(!bin)
		return -EC_BAD_SECTION_LENGTH;
	len = *bin++;
	if(len < 240) {
	} else if(len <= 252) {
		if(!bin)
			return -EC_BAD_SECTION_LENGTH;
		len = ((len & 0xF)<<8) + *bin++;
	} else if(len == 253) {
		if(bin < 2)
			return -EC_BAD_SECTION_LENGTH;
		len = (bin[0]<<8) + bin[1];
		bin += 2;
	} else if(len == 254) {
		if(bin < 3)
			return -EC_BAD_SECTION_LENGTH;
		len = (bin[0]<<16) + (bin[1]<<8) + bin[2];
		bin += 3;
	} else {
		if(bin < 4)
			return -EC_BAD_SECTION_LENGTH;
		len = (bin[0]<<24) + (bin[1]<<16) + (bin[2]<<8) + bin[3];
		bin += 4;
		if(len > MAXPACKETSIZE)
			return -EC_BAD_VALUE_LENGTH;
	}
	return 0;
}



char *EncodeLength(char *p, uint32_t len) {
	if(len < 240) {
		p[0] = len;
		return p+1;
	} else if(len < (13<<8)) {
		p[0] = 0xF0 + (len>>8);
		p[1] = len & 0xFF;
		return p+2;
	} else if(len < (1<<16)) {
		p[0] = 253;
		p[1] = len >> 8;
		p[2] = len & 0xFF;

		return p+3;
	} else if(len < (1<<24)) {
		p[0] = 254;
		p[1] = len >> 16;
		p[2] = len >> 8;
		p[3] = len & 0xFF;
		return p+4;
	} else {
		p[0] = 255;
		p[1] = len >> 24;
		p[2] = len >> 16;
		p[3] = len >> 8;
		p[4] = len & 0xFF;
		return p+5;
	}
	return 0;
}

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

		for(int i=0; i<n; i++)
			p[i] = t[n-1-i];

		p += n;
		break;

	case DField::Float:
		char buf[sizeof(long double)*2+8];
		if(sprintf(buf, __FLTFMT__, (long double)v->flt) >= (int)sizeof(buf))
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

int EncodedBytesLength(uint32_t n) {
	if(n < 240) return 1;
	if(n < (13<<8))  return 2;
	if(n < (1<<16)) return 3;
	if(n < (1<<24)) return 4;
	return 5;
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
		if(sprintf(b, __FLTFMT__, (long double)v->flt) >= (int)sizeof(b))
			return 5;
		return 2+strlen(b);
	case DField::String:
	case DField::Binary:
		if(v->str.ptr==NULL) return 1;
		return v->str.len+1+EncodedBytesLength(v->str.len+1);
	}
	return 0;
}

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
 * decode value by type
 * datavalue format:
 * 	<LEN> <VAL>
 */
int DecodeDataValue(CBinary &bin, CValue &val, int type) {
	uint8_t *p;
	uint32_t len;
	int rv;

	rv = DecodeLength(bin, len);
	if(rv){
		return rv;
	}
	
	if(bin.len < (int64_t)len) return -EC_BAD_SECTION_LENGTH;

	switch(type) {
	case DField::None:
	    break;

	case DField::Signed:
	case DField::Unsigned:
	    /* integer always encoded as signed value */
	    if(len==0 || len > 8) return -EC_BAD_VALUE_LENGTH;
	    p = (uint8_t *)bin.ptr + 1;
	    int64_t s64;
	    s64 = *(int8_t *)bin.ptr;
	    switch(len) {
	    case 8:
	    	s64 = (s64<<8) | *p++;
	    case 7:
	    	s64 = (s64<<8) | *p++;
	    case 6:
	    	s64 = (s64<<8) | *p++;
	    case 5:
	    	s64 = (s64<<8) | *p++;
	    case 4:
	    	s64 = (s64<<8) | *p++;
	    case 3:
	    	s64 = (s64<<8) | *p++;
	    case 2:
	    	s64 = (s64<<8) | *p++;
	    }
	    val.Set(s64);
	    break;

	case DField::Float:
	    /* float value encoded as %A string */
	    if(len < 3) return -EC_BAD_VALUE_LENGTH;
	    if(bin[len-1] != '\0') return -EC_BAD_FLOAT_VALUE;
	    if(!strcmp(bin.ptr, "NAN"))
	    	val.flt = len;//NAN  ¸ÄÎªlen
	    else if(!strcmp(bin.ptr, "INF"))
	    	val.flt = INFINITY;
	    else if(!strcmp(bin.ptr, "-INF"))
	    	val.flt = -INFINITY;
	    else {
		long double ldf;
		if(sscanf(bin.ptr, __FLTFMT__, &ldf)!=1)
	    	    return -EC_BAD_FLOAT_VALUE;
		val.flt = ldf;
	    }
	    break;

	case DField::String:
	    /* NULL encoded as zero length, others padded '\0' */
	    if(len==0) {
#if CONVERT_NULL_TO_EMPTY_STRING
		val.Set(bin.ptr-1, 0);
#else
	    	val.Set(NULL, 0);
#endif
	    } else {
		if(bin[len-1] != '\0') return -EC_BAD_STRING_VALUE;
		val.Set(bin.ptr, len-1);
	    }
	    break;

	case DField::Binary:
	    /* NULL encoded as zero length, others padded '\0' */
	    if(len==0) {
#if CONVERT_NULL_TO_EMPTY_STRING
		val.Set(bin.ptr-1, 0);
#else
	    	val.Set(NULL, 0);
#endif
	    } else {
		if(bin[len-1] != '\0') return -EC_BAD_STRING_VALUE;
		val.Set(bin.ptr, len-1);
	    }
	    break;
	}
	bin += len;
	return 0;
}

/*
 * Tag format:
 * <ID> <LEN> <VALUE>
 * 	ID: 1 bytes
 * 	LEN: Length encoding
 * 	VALUE: DataValue encoding, predefined type
 * Simpel Section format:
 * 	<TAG> <TAG> ...
 */
int DecodeSimpleSection(CBinary &bin, CSimpleSection &ss, uint8_t kt) {
	uint8_t mask[32];
	FIELD_ZERO(mask);
	while(!!bin)
	{
		int id = *bin++;

		if(FIELD_ISSET(id, mask)) return -EC_DUPLICATE_TAG;
		int type = id==0 ? kt : ss.TagType(id);
		//CValue val;
		//int rv = DecodeDataValue(bin, val, type);
		/* avoid one more copy of tag CValue */
		/* int(len, value):  buf -> local; buf -> local -> tag */
		/* str(len, value):  buf -> local -> tag; buf -> tag */
		int rv = DecodeDataValue(bin, *ss.GetThisTag(id), type);
		if(rv){
			printf("decode tag[id:%d] error: %d", id, rv);
			return rv;
		}
		if(type != DField::None) {
		    //ss.SetTag(id, val);
		    /* no need of check if none type section and check if duplicate tag */
		    ss.SetTagMask(id);
		    FIELD_SET(id, mask);
		}
	}
	return 0;
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


int DecodeSimpleSection(char *p, int l, CSimpleSection &ss, uint8_t kt) {
	CBinary bin = { l, p };
	return DecodeSimpleSection(bin, ss, kt);
}


/*
 * two form of field Id
 * <ID>			ID>0, by ID
 * 0 <LEN> <NAME>	byname, LEN is NAME length, no '\0'
 */
int DecodeFieldId(CBinary &bin, uint8_t &id, const CTableDefinition *tdef, int &needDefinition) {
	if(!bin)
		return -EC_BAD_SECTION_LENGTH;
	uint8_t n = *bin++;
	if(n) {
	    id=n;
	} else {
	    if(!bin)
		return -EC_BAD_SECTION_LENGTH;
	    n = *bin++;
		if(n == 0){
			id = n;
		}
		else{
		    if(bin < n)
			return -EC_BAD_SECTION_LENGTH;
		    int fid;
//		    if(n <= 0 || (fid = tdef->FieldId(bin.ptr, n)) <= 0){
			if(n <= 0 || (fid = tdef->FieldId(bin.ptr, n)) < 0) {// allow select key-field
				printf("bad field name: %s\n", bin.ptr);
				return -EC_BAD_FIELD_NAME;
			}
		    id = fid;
		    bin += n;
		    needDefinition = 1;
		}
	}
	return 0;
}
