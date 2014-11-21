#ifndef __CH_DECODE_H__
#define __CH_DECODE_H__

#include "field_api.h"
#include "field.h"

extern int DecodeLength(CBinary &bin, uint32_t &len);
extern char *EncodeLength(char *p, uint32_t len);
extern char *EncodeDataValue(char *p, const CValue *v, int type);
extern int EncodedBytesLength(uint32_t n);
extern int EncodedBytesDataValue(const CValue *v, int type);
extern int EncodedBytesSimpleSection(const CSimpleSection &, uint8_t);
extern char *EncodeSimpleSection(char *p, const CSimpleSection &, uint8_t);
extern int DecodeDataValue(CBinary &bin, CValue &val, int type);
extern int EncodedBytesFieldValue(const CFieldValueByName &);
extern int EncodedBytesFieldValue(const CFieldValue &);
extern int EncodedBytesFieldSet(const CFieldSetByName &);
extern char * EncodeFieldValue(char *, const CFieldValueByName &);
extern char * EncodeFieldSet(char *p, const CFieldSetByName &);

//deecode
extern int DecodeSimpleSection(CBinary &, CSimpleSection &, uint8_t);
extern int DecodeSimpleSection(char *, int, CSimpleSection &, uint8_t);
extern int DecodeFieldId(CBinary &, uint8_t &id, const CTableDefinition *tdef, int &needDefinition);
#endif