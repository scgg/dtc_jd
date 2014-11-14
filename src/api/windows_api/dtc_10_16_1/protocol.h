#ifndef __CH_PROTOCOL_H_
#define __CH_PROTOCOL_H_
#include <stdint.h>


#define MAXFIELDS_PER_TABLE	255
#define MAXPACKETSIZE	(64<<20)

class DField {
public:
    enum {
	None=0,	// undefined
	Signed=1,	// Signed Integer
	Unsigned=2,	// Unsigned Integer
	Float=3,	// FloatPoint
	String=4,	// String, case insensitive, null ended
	Binary=5,	// opaque binary data
	TotalType
    };

    enum {
	Set = 0,
	Add = 1,
	SetBits = 2,
	OR = 3,
	TotalOperation
    };

    enum {
	EQ = 0,
	NE = 1,
	LT = 2,
	LE = 3,
	GT = 4,
	GE = 5,
	TotalComparison
    };
};

class DRequest {
public:
	enum {
	    Nop = 0,
	    ResultCode = 1,
	    ResultSet = 2,
	    //��������
	    SvrAdmin = 3,
	    Get = 4,
	    Purge = 5,
	    Insert = 6,
	    Update = 7,
	    Delete = 8,
	    Replace = 12,
	    Flush = 13,
	    Invalidate = 14, // OBSOLETED
	    Monitor = 15,
	};

	class Flag {
	    public:
		enum {
		    KeepAlive = 1,
		    NeedTableDefinition = 2,
		    NoCache = 4,
		    NoResult = 8,
		    //ͬ��������
		    NoNextServer = 16,
			MultiKeyValue = 32,
			AdminTable = 64,
		};
	};

	class Section {
	    public:
		enum {
		    VersionInfo = 0,
		    TableDefinition = 1,
		    RequestInfo = 2,
		    ResultInfo = 3,
		    UpdateInfo = 4,
		    ConditionInfo = 5,
		    FieldSet = 6,
		    ResultSet = 7,
		    Total
		};
	};

    class ServerAdminCmd
    {
    public:
		enum CMD{
			RegisterHB = 1,
			LogoutHB = 2,
			GetKeyList = 3,
			GetUpdateKey = 4,
			GetRawData = 5,
			ReplaceRawData = 6,
			AdjustLRU = 7,
			VerifyHBT = 8,
			GetHBTime = 9,
			SET_READONLY = 10,
			SET_READWRITE	= 11,
			QueryServerInfo = 12,
			NodeHandleChange = 13,
			Migrate = 14,
            ReloadClusterNodeList = 15,
            SetClusterNodeState = 16,
            ChangeNodeAddress = 17,
            GetClusterState = 18,
	    PurgeForHit     = 19,
	       QUERY_MEM_INFO     = 20,
		};
    };
};

struct CPacketHeader {
	uint8_t version;
	uint8_t scts;
	uint8_t flags;
	uint8_t cmd;
	uint32_t len[DRequest::Section::Total];
};

struct CServerInfo {
	uint8_t  version;
	uint8_t  reserved[3];
	uint32_t binlog_id;
	uint32_t binlog_off;
	uint64_t memsize;
	uint64_t datasize;
};

#endif
