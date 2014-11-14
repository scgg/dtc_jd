#include <stdint.h>

#include "value.h"
#include "tabledef.h"
#include "task_pkey.h"

void CTaskPackedKey::UnpackKey(const CTableDefinition *tdef, const char *ptrKey, CValue *val) {
	if(tdef->KeyFormat()==0) {
		val->bin.ptr = (char *)ptrKey+1;
		val->bin.len = *(unsigned char *)ptrKey;
	} else {
		for(int i=0; i<tdef->KeyFields(); val++, ptrKey += tdef->FieldSize(i), i++)
		{
			const int size = tdef->FieldSize(i);
#if __BYTE_ORDER == __LITTLE_ENDIAN
			val->s64 = tdef->FieldType(i)==DField::Unsigned ? 0 :
				ptrKey[size-1] & 0x80 ? -1 : 0;
			memcpy((char *)&val->s64, ptrKey, size);
#elif __BYTE_ORDER == __BIG_ENDIAN
			val->s64 = tdef->FieldType(i)==DField::Unsigned ? 0 :
				ptrKey[0] & 0x80 ? -1 : 0;
			memcpy((char *)&val->s64 + (sizeof(uint64_t)-size), ptrKey, size);
#else
#error unkown endian
#endif
		}
	}
}

int CTaskPackedKey::BuildPackedKey(const CTableDefinition *tdef, const CValue* pstKeyValues, unsigned int uiBufSize, char packKey[])
{
	if(tdef->KeyFormat()==0){ // single string key
		if(pstKeyValues->str.len > (int)tdef->FieldSize(0))
			return -1;
		if((int)uiBufSize < pstKeyValues->str.len+1)
			return -2;
		if(tdef->FieldType(0) == DField::String) {
			packKey[0] = pstKeyValues->str.len;
			for(int i=0; i<pstKeyValues->str.len; i++)
				packKey[i+1] = INTERNAL_TO_LOWER(pstKeyValues->str.ptr[i]);
		} else {
			packKey[0] = pstKeyValues->str.len;
			memcpy(packKey+1, pstKeyValues->str.ptr, pstKeyValues->str.len);
		}
	}
	else if(tdef->KeyFields()==1){ // single int key
		if(uiBufSize < (unsigned int)(tdef->KeyFormat()))
			return -3;

		if(StoreValue(packKey, pstKeyValues->u64,
			tdef->FieldType(0),
			tdef->FieldSize(0)) < 0)
		{
			return -4;
		}
	}
	else{ // multi-int key
		if((int)uiBufSize < tdef->KeyFormat())
			return -5;
			
		for(int i=0; i<tdef->KeyFields(); i++) {
			if(StoreValue(packKey + tdef->FieldOffset(i),
						(pstKeyValues+i)->u64,
						tdef->FieldType(i),
						tdef->FieldSize(i)) < 0)
			{
				return -6;
			}
		}
	}
	
	return 0;
}

unsigned long CTaskPackedKey::CalculateBarrierKey(const CTableDefinition *tdef, const char *pkey)
{
		unsigned const char *p = (unsigned const char *)pkey;
		int len = tdef->KeyFormat();
		if(len==0) len = *p++;

		unsigned long h = 0;
		unsigned char c;
		while(len-- > 0 ) {
			c = *p++;
			h = h * 11111 + (c<<4)+(c>>4);
		}
		return h;
}
