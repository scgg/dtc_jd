#ifndef __ADMIN_TDEF_H__
#define __ADMIN_TDEF_H__

class CTableDefinition;
class CHotBackup{
public:
    //type
    enum {
        SYNC_LRU    =1,
        SYNC_INSERT =2,
        SYNC_UPDATE =4,
        SYNC_PURGE  =8,
        SYNC_DELETE =16,
    };

    //flag
	enum {
        NON_VALUE   = 1,
		HAS_VALUE   = 2,
        EMPTY_NODE  = 4,
        KEY_NOEXIST = 8,
	};

};

class CMigrate{
public:
    //type
    enum {
        FROM_CLIENT    =1,
        FROM_SERVER =2
    };

    //flag
	enum {
        NON_VALUE   = 1,
		HAS_VALUE   = 2,
        EMPTY_NODE  = 4,
        KEY_NOEXIST = 8,
	};

};
extern CTableDefinition* BuildHotBackupTable(void);

#endif
