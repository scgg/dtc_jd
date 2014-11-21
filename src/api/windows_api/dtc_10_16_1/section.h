#ifndef __CH_SECTION_H__
#define __CH_SECTION_H__

//#include "memcheck.h"
#include <string.h>
#include <math.h>
#include "cache_error.h"
#include <new>
#include "keylist.h"
#include "value.h"
#include "protocol.h"

//#include "log.h"

#define MAX_STATIC_SECTION 20

/*
 * tag: a pair of id:value, the value type if predefined
 * section: a set of id:value
 */
struct CSectionDefinition {
	uint8_t sectionId;
	uint8_t maxTags;
	uint8_t tagType[256];

	uint8_t TagType(uint8_t i) const { return tagType[i]; };
	int MaxTags(void) const { return maxTags; }
	int SectionId(void) const { return sectionId; }
};

class CSimpleSection {
private:
	const CSectionDefinition *definition;
	uint8_t numTags;
#if MAX_STATIC_SECTION
	uint8_t fieldMask[(MAX_STATIC_SECTION+7)/8];
#else
	uint8_t fieldMask[32];
#endif

public:
#if MAX_STATIC_SECTION
	CValue tagValue[MAX_STATIC_SECTION];
#else
	CValue *tagValue;
#endif

	CSimpleSection(const CSectionDefinition &d) :
	    definition(&d), numTags(0)
	{
#if MAX_STATIC_SECTION
	    for(int i=0; i<d.MaxTags(); i++)
	    	tagValue[i].Set(NULL, 0);
#else
	    tagValue = (CValue *)calloc(d.MaxTags(), sizeof(CValue));
	    if(tagValue==NULL) throw std::bad_alloc();
#endif
	    FIELD_ZERO(fieldMask);
	}

	virtual ~CSimpleSection() {
#if !MAX_STATIC_SECTION
		FREE_IF(tagValue);
#endif
	}
	CSimpleSection(const CSimpleSection &orig) {
#if !MAX_STATIC_SECTION
	    tagValue = (CValue *)malloc(orig.definition->MaxTags(), sizeof(CValue));
	    if(tagValue==NULL) throw std::bad_alloc();
#endif
	    Copy(orig);
	}
	void Copy(const CSimpleSection &orig) {
	    for(int i=0; i<orig.definition->MaxTags(); i++)
	    	tagValue[i] = orig.tagValue[i];
	    memcpy(fieldMask, orig.fieldMask, sizeof(fieldMask));
	    definition = orig.definition;
	    numTags = orig.numTags;
	}

	/* newman: pool */
	inline virtual void Clean()
	{
	    memset(tagValue, 0, sizeof(CValue) * definition->MaxTags());
	    FIELD_ZERO(fieldMask);
        numTags = 0;
	}

	int MaxTags(void) const {
	    return definition->MaxTags();
	}

	int NumTags(void) const {
	    return numTags;
	}

	int SectionId(void) const {
	    return definition->SectionId();
	}

	uint8_t TagType(uint8_t id) const {
	    return definition->TagType(id);
	}

	int TagPresent(uint8_t id) const {
	    return id>=MAX_STATIC_SECTION?0:FIELD_ISSET(id, fieldMask);
	}

	void SetTag(uint8_t id, const CValue &val) {
	    if(TagType(id) != DField::None) {
		tagValue[id] = val;
		if(!FIELD_ISSET(id, fieldMask)) {
		    numTags++;
		    FIELD_SET(id, fieldMask);
		}
	    }
	}

	/* no check of dumplicate tag */
	void SetTagMask(uint8_t id)
	{
	    numTags++;
	    FIELD_SET(id, fieldMask);
	}

	void SetTag(uint8_t id, int64_t val) {
	    //if(TagType(id) == DField::Signed)
		SetTag(id, CValue::Make(val));
	}
	void SetTag(uint8_t id, uint64_t val) {
	    //if(TagType(id) == DField::Unsigned)
		SetTag(id, CValue::Make(val));
	}
	void SetTag(uint8_t id, int32_t val) {
	    if(TagType(id) == DField::Signed)
		SetTag(id, CValue::Make(val));
	    else if(TagType(id)==DField::Unsigned) {
	    	if(val<0) val = 0;
		SetTag(id, CValue::Make((uint64_t)val));
	    }
	}
	void SetTag(uint8_t id, uint32_t val) {
        //fix Unsigned to Signed
	    if(TagType(id)==DField::Signed)
		SetTag(id, CValue::Make((int64_t)val));
	    else if(TagType(id)==DField::Unsigned)
		SetTag(id, CValue::Make(val));
	}
	void SetTag(uint8_t id, double val) {
	    if(TagType(id)==DField::Float)
		SetTag(id, CValue::Make(val));
	}
	void SetTag(uint8_t id, const char *val, int len) {
	    if(TagType(id)==DField::String || TagType(id)==DField::Binary)
		SetTag(id, CValue::Make(val, len));
	}
	void SetTag(uint8_t id, const char *val) {
	    if(TagType(id)==DField::String || TagType(id)==DField::Binary)
		SetTag(id, CValue::Make(val));
	}

	const CValue *GetTag(uint8_t id) const {
	    return TagPresent(id) ? &tagValue[id] : NULL;
	}

	/* no check */
	CValue * GetThisTag(uint8_t id){
	    return &tagValue[id];
	}

	const CValue *Key(void) const {
		return TagPresent(0) ? &tagValue[0] : NULL;
	}
	void SetKey(const CValue &k) { SetTag(0, k); }
};

extern const CSectionDefinition versionInfoDefinition;
extern const CSectionDefinition requestInfoDefinition;
extern const CSectionDefinition resultInfoDefinition;

class CVersionInfo : public CSimpleSection {
    public:
	CVersionInfo() : CSimpleSection(versionInfoDefinition) {}
	~CVersionInfo() {}
	CVersionInfo(const CVersionInfo &orig) : CSimpleSection(orig) { }

	const CBinary &TableName(void) const { return tagValue[1].str; }
	void SetTableName(const char *n) { SetTag(1, n);}
	const CBinary &TableHash(void) const { return tagValue[4].str; }
	void SetTableHash(const char *n) { SetTag(4, n, 16); }
	const CBinary &DataTableHash(void) const { return tagValue[2].str; }
	void SetDataTableHash(const char *n) { SetTag(2, n, 16); }
	
	const CBinary &CTLibVer(void) const { return tagValue[6].str; }
	const int CTLibIntVer(void) const { 
		int major=3;//, minor=0, micro=0;

		/* 3.x系列的批量拆包之后没有version信息 */
		if(NULL == CTLibVer().ptr)
		{
			printf("multi task have no version info");
			return major;
		}

		/*replace sscanf by code below for promoting performance,add by foryzhou 2010.09.01*/
		char buf = CTLibVer().ptr[7];
		if (buf >= '1' && buf <= '9') 
		{
				major = buf-'0';
				printf("client major version:%d,version string:%s",major,CTLibVer().ptr);
				return major;
		}
		else
		{
				printf("unknown client api version: %s", CTLibVer().ptr);
				return major;
		}
				/*
		if(sscanf(CTLibVer().ptr, "ctlib-v%d.%d.%d", &major, &minor, &micro) != 3)
		{
			//log_debug("unknown client api version: %s", CTLibVer().ptr);
			return major;
		}
		
		//log_debug("client api version: %d.%d.%d", major, minor, micro);
		return major;
		*/
	}
	
	int KeyType(void) const { return tagValue[9].u64; }
	void SetKeyType(int n) { SetTag(9, n); }
#if MAX_STATIC_SECTION >=1 && MAX_STATIC_SECTION < 10
#error MAX_STATIC_SECTION must >= 10
#endif

	const uint64_t SerialNr(void) const { return tagValue[3].u64; }
	void SetSerialNr(uint64_t u) { SetTag(3, u); }
	const int KeepAliveTimeout(void) const {
		return (int)tagValue[8].u64;
	}
	void SetKeepAliveTimeout(uint32_t u) { SetTag(8, u); }
	
	void SetHotBackupID(uint64_t u){ SetTag(14, u); }
	uint64_t HotBackupID() const { return tagValue[14].u64; }
	void SetMasterHBTimestamp(int64_t t) { SetTag(15, t); }
	int64_t MasterHBTimestamp(void) const { return tagValue[15].s64; }
	void SetSlaveHBTimestamp(int64_t t) { SetTag(16, t); }
	int64_t SlaveHBTimestamp(void) const { return tagValue[16].s64; }
    uint64_t GetAgentClientId() const { return tagValue[17].u64; }
    void SetAgentClientId(uint64_t id) { SetTag(17, id); }

    /* date:2014/06/04, author:xuxinxin */
    void SetAccessKey(const char *token) { SetTag(18, token);}
    const CBinary &AccessKey(void) const { return tagValue[18].str; }
};

class CRequestInfo : public CSimpleSection {
    public:
	CRequestInfo() : CSimpleSection(requestInfoDefinition) {}
	~CRequestInfo() {}
	CRequestInfo(const CRequestInfo &orig) : CSimpleSection(orig) { }

	uint64_t ExpireTime(int version) const {
		/* server内部全部按照ms单位来处理超时 */
		if(version >= 3)
			/* 3.x 系列客户端发送的超时时间单位：us */
			return tagValue[1].u64 >>10;
		else
			/* 2.x 系列客户端发送的超时时间单位: ms */
			return tagValue[1].u64;
	}
	void SetTimeout(uint32_t n) { SetTag(1, (uint64_t)n<<10); }
	uint32_t LimitStart(void) const { return tagValue[2].u64; }
	uint32_t LimitCount(void) const { return tagValue[3].u64; }
	void SetLimitStart(uint32_t n) { SetTag(2, (uint64_t)n); }
	void SetLimitCount(uint32_t n) { SetTag(3, (uint64_t)n); }

	uint32_t AdminCode() const { return tagValue[7].u64; }
	void SetAdminCode(uint32_t code) { SetTag(7, (uint64_t)code); }
};

class CResultInfo : public CSimpleSection {
    private:
	char *szErrMsg;
	char *s_info;
    public:
	CResultInfo() : CSimpleSection(resultInfoDefinition), szErrMsg(NULL),s_info(NULL) {}
	~CResultInfo() { FREE_CLEAR(szErrMsg); FREE_CLEAR(s_info);}
	CResultInfo(const CResultInfo &orig) : CSimpleSection(orig) {
		if(orig.szErrMsg) {
			strcpy(szErrMsg,orig.szErrMsg);
			SetTag(3, szErrMsg, strlen(szErrMsg));
		}
		if(orig.s_info) {
			s_info = NULL;
			SetServerInfo((CServerInfo *)orig.s_info);
		}
	}

	/* newman: pool */
	inline virtual void Clean()
	{
	    CSimpleSection::Clean();
	    FREE_CLEAR(szErrMsg);
	    FREE_CLEAR(s_info);
	}
	void Copy(const CResultInfo &orig) {
		CSimpleSection::Copy(orig);
		FREE_CLEAR(szErrMsg);
		if(orig.szErrMsg) {
			strcpy(szErrMsg,orig.szErrMsg);
			SetTag(3, szErrMsg, strlen(szErrMsg));
		}
		FREE_CLEAR(s_info);
		if(orig.s_info) {
			SetServerInfo((CServerInfo *)orig.s_info);
		}
	}

	void SetError(int err, const char *from, const char *msg) {
	    SetTag(1, err);
	    if(from) SetTag(2, (char *)from, strlen(from));
	    if(msg) SetTag(3, (char *)msg, strlen(msg));
	}
	void SetErrorDup(int err, const char *from, const char *msg) {
	    FREE_IF(szErrMsg);
	    strcpy(szErrMsg,msg);
	    SetTag(1, err);
	    if(from) SetTag(2, (char *)from, strlen(from));
	    if(msg) SetTag(3, szErrMsg, strlen(szErrMsg));
	}
	int ResultCode(void) const { return tagValue[1].s64; }
	const char *ErrorFrom(void) const { return tagValue[2].str.ptr; }
	const char *ErrorMessage(void) const { return tagValue[3].str.ptr; }
	uint64_t AffectedRows(void) const { return tagValue[4].s64; }
	void SetAffectedRows(uint64_t nr) { SetTag(4, nr); }
	uint64_t TotalRows(void) const { return tagValue[5].s64; }
	void SetTotalRows(uint64_t nr) { SetTag(5, nr); }
	uint64_t InsertID(void) const { return tagValue[6].u64; }
	void SetInsertID(uint64_t nr) { SetTag(6, nr); }
	char* ServerInfo(void) const{
		if(tagValue[7].str.len != sizeof(CServerInfo))
			return NULL;
		return tagValue[7].str.ptr;
	}

	void SetServerInfo(CServerInfo *si) {
	    FREE_IF(s_info);
	    int len = sizeof(CServerInfo);
	    s_info = (char *)CALLOC(1, len);
	    memcpy(s_info, si, len);
	    SetTag(7, s_info, len);
	    return;
	}
	uint32_t Timestamp(void) const { return tagValue[8].s64; }
	void SetTimestamp(uint32_t nr) { SetTag(8, nr); }
	/*set hit flag by tomchen 20140604*/
	void SetHitFlag(uint32_t nr) { SetTag(9, nr); }
	uint32_t HitFlag(){return tagValue[9].s64;};


	
		
};

#endif
