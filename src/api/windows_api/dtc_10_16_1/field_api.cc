#include <malloc.h>
#include <stdio.h>
//#include "field.h"
#include "field_api.h"
#include "cache_error.h"
//#include <string.h>
//#include "bitsop.h"
//#include "log.h"


/*
 * resolve fieldname at client side
 * 	solved: all field has resolved
 */


int CFieldValueByName::Resolve(const CTableDefinition *tdef, int force) {

	if(force)
		solved = 0;
	else if(solved)
		return 0;
	else if(numFields==0)
		return 0;

	if(tdef==NULL)
		return -EINVAL;

	for(int i=0; i<numFields; i++) {
	    if(fieldValue[i].name==NULL)
		continue;
	    const int fid = tdef->FieldId(fieldValue[i].name, fieldValue[i].nlen);
//	    if(fid <= 0)
		if(fid < 0)
		return -EC_BAD_FIELD_NAME;
	    fieldValue[i].fid = fid;
	}
	solved = 1;

	return 0;
}




/*
 * resolve fieldname at client side
 * 	solved: all field has resolved
 */
void CFieldValueByName::Unresolve(void) {

	for(int i=0; i<numFields; i++) {
		if(fieldValue[i].name!=NULL) {
			fieldValue[i].fid = INVALID_FIELD_ID;
			solved = 0;
		}
	}

}

/*
 * resolve fieldname at client side
 * 	solved: all field has resolved
 */


int CFieldSetByName::Resolve(const CTableDefinition *tdef, int force) {

	if(force)
		solved = 0;
	else if(solved)
		return 0;
	else if(numFields==0)
		return 0;
	if(tdef==NULL)
		return -EINVAL;
	for(int i=0; i<numFields; i++) {
	    if(fieldValue[i].name==NULL)
		continue;
	    const int fid = tdef->FieldId(fieldValue[i].name, fieldValue[i].nlen);
//	    if(fid<=0)
		if(fid < 0)
		return -EC_BAD_FIELD_NAME;
	    fieldValue[i].fid = fid;
	}
	solved = 1;

	return 0;
}



/*
 * resolve fieldname at client side
 * 	solved: all field has resolved
 */
void CFieldSetByName::Unresolve(void) {

	for(int i=0; i<numFields; i++) {
		if(fieldValue[i].name!=NULL) {
			fieldValue[i].fid = INVALID_FIELD_ID;
			solved = 0;
		}
	}
	
}

const uint8_t *CFieldSetByName::VirtualMap(void) const {

	if(maxvid==0)
		return NULL;
	if(solved==0)
		return NULL;
	uint8_t *m = (uint8_t *)calloc(1, maxvid);
	if(m==NULL)
		throw std::bad_alloc();
	for(int i=0; i<numFields; i++) {
	    if(fieldValue[i].vid)
		m[fieldValue[i].vid-1] = fieldValue[i].fid;
	}
	return m;

	return 0;
}

int CFieldSetByName::AddField(const char *name, int vid) {
	int nlen = strlen(name);
	if(nlen >= 1024) {
		return -EC_BAD_FIELD_NAME;
	}
	if(vid < 0 || vid >= 256)
	{
	    vid = 0;
	    return -EINVAL;
	} else if(vid) {
	    for(int i=0; i<numFields; i++)
	    {
	    	if(fieldValue[i].vid == vid)
			return -EINVAL;
	    }
	}
	if(numFields==maxFields) {
	    if(maxFields==255)
		return -E2BIG;
	    int n = maxFields + 8;
	    if(n > 255) n = 255;
	    fieldV *p=new fieldV;
	    if(fieldValue==NULL) {
	    	p = (fieldV*)malloc(n*sizeof(fieldV));
	    } else {
	    	p = (fieldV*)realloc(fieldValue, n*sizeof(fieldV));
	    }
	    if(p==NULL) throw std::bad_alloc();
	    fieldValue = p;
	    maxFields = n;
	}

	char *str = (char *)malloc(nlen+1);
	if(str==NULL) throw std::bad_alloc();
	memcpy(str, name, nlen+1);
	fieldValue[numFields].name = str;
	fieldValue[numFields].nlen = nlen;
//	fieldValue[numFields].fid = 0;
	fieldValue[numFields].fid = INVALID_FIELD_ID; // allow select key-field
	fieldValue[numFields].vid = vid;
	if(vid > maxvid) maxvid = vid;
	solved = 0;
	numFields++;
	return 0;
}

int CFieldValueByName::AddValue(const char *name, uint8_t op, uint8_t type, const CValue &val) {

	int nlen = strlen(name);
	if(nlen >= 1024) {
		return -EC_BAD_FIELD_NAME;
	}
	if(numFields==maxFields) {
	    if(maxFields==255)
		return -E2BIG;
	    int n = maxFields + 8;
	    if(n > 255) n = 255;
	    FieldVal* p=NULL;
	    if(fieldValue==NULL) {
	    	p = (FieldVal*)malloc(n*sizeof(*fieldValue));
	    } else {
	    	p = (FieldVal*)realloc(fieldValue, n*sizeof(*fieldValue));
	    }
	    if(p==NULL) throw std::bad_alloc();
	    fieldValue = p;
	    maxFields = n;
	}

	char *str = (char *)MALLOC(nlen+1);
	if(str==NULL) throw std::bad_alloc();
	memcpy(str, name, nlen+1);
	fieldValue[numFields].name = str;
	fieldValue[numFields].nlen = nlen;
	fieldValue[numFields].type = type;
	fieldValue[numFields].fid = INVALID_FIELD_ID;
	fieldValue[numFields].oper = op;
	fieldValue[numFields].val = val;
	if(type==DField::String || type==DField::Binary)
	{
		if(val.bin.ptr!=NULL)
		{
			char *p = (char *)MALLOC(val.bin.len+1);
			if(p==NULL) throw std::bad_alloc();
			memcpy(p, val.bin.ptr, val.bin.len);
			p[val.bin.len] = '\0';
			fieldValue[numFields].val.bin.ptr = p;
		}
	}
	solved = 0;
	numFields++;

	return 0;
}
