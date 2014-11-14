#ifndef __CH_DECODE_H__
#define __CH_DECODE_H__

/*
 * Base Decode/Encode routines:
 *	p = Encode...(p, ...);	// encode object and advance pointer
 *	EncodedBytes...(...);	// calculate encoded object size
 *	Decode...(...);		// Decode objects
 */
#include "value.h"
#include "tabledef.h"
#include "section.h"
#include "field.h"
#include "field_api.h"

#define __FLTFMT__	"%LA"
static inline char *EncodeDataType(char *p, uint8_t type) {
	*p++ = type==DField::Unsigned ? DField::Signed : type;
	return p;
}

extern int DecodeLength(CBinary &bin, uint32_t &len);
extern char *EncodeLength(char *p, uint32_t len);
extern int EncodedBytesLength(uint32_t n);

extern int DecodeDataValue(CBinary &bin, CValue &val, int type);
extern char *EncodeDataValue(char *p, const CValue *v, int type);
extern int EncodedBytesDataValue(const CValue *v, int type);

extern int DecodeFieldId(CBinary &, uint8_t &id, const CTableDefinition *tdef, int &needDefinition);
extern int DecodeSimpleSection(CBinary &, CSimpleSection &, uint8_t);
extern int DecodeSimpleSection(char *, int, CSimpleSection &, uint8_t);

extern int EncodedBytesSimpleSection(const CSimpleSection &, uint8_t);
extern char *EncodeSimpleSection(char *p, const CSimpleSection &, uint8_t);
extern int EncodedBytesFieldSet(const CFieldSet &);
extern char * EncodeFieldSet(char *p, const CFieldSet &);
extern int EncodedBytesFieldValue(const CFieldValue &);
extern char * EncodeFieldValue(char *, const CFieldValue &);

extern int EncodedBytesMultiKey(const CValue *v, const CTableDefinition *tdef);
extern char *EncodeMultiKey(char *, const CValue *v, const CTableDefinition *tdef);
class CFieldSetByName;
extern int EncodedBytesFieldSet(const CFieldSetByName &);
extern char * EncodeFieldSet(char *p, const CFieldSetByName &);
extern int EncodedBytesFieldValue(const CFieldValueByName &);
extern char * EncodeFieldValue(char *, const CFieldValueByName &);

#endif
