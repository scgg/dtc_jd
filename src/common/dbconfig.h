#ifndef _H_TTC_DB_CONFIG_H_
#define _H_TTC_DB_CONFIG_H_

#include "tabledef.h"
#include "config.h"
#include <stdint.h>

#define MAXDB_ON_MACHINE	1000
#define GROUPS_PER_MACHINE	4
#define GROUPS_PER_ROLE		3
#define ROLES_PER_MACHINE	2
#define MACHINEROLESTRING	"ms?????"

#define DB_FIELD_FLAGS_READONLY 1
#define DB_FIELD_FLAGS_UNIQ 2
#define DB_FIELD_FLAGS_DESC_ORDER 4
#define DB_FIELD_FLAGS_VOLATILE 8
#define DB_FIELD_FLAGS_DISCARD 0x10

/* 默认key-hash so文件名及路径 */
#define DEFAULT_KEY_HASH_SO_NAME	"../lib/key-hash.so"
/* key-hash接口函数 */
typedef uint64_t (*key_hash_interface)(const char *key, int len, int left, int right);

enum {
	INSERT_ORDER_LAST = 0,
	INSERT_ORDER_FIRST = 1,
	INSERT_ORDER_PURGE = 2
};

enum {
	BY_MASTER,
	BY_SLAVE,
	BY_DB,
	BY_TABLE,
	BY_KEY,
	BY_FIFO
};

typedef enum
{
    DUMMY_HELPER      = 0, 
    TTC_HELPER,
    MYSQL_HELPER,
    TDB_HELPER,
    CUSTOM_HELPER,
}HELPERTYPE;

struct CMachineConfig {
	struct {
		const char *addr;
		const char *user;
		const char *pass;
		const char *optfile;
		//DataMerge Addr
		const char *dm;
		char path[32];
	} role[GROUPS_PER_ROLE];

	HELPERTYPE helperType;
	int  mode;
	uint16_t dbCnt;
	uint16_t procs;
	uint16_t dbIdx[MAXDB_ON_MACHINE];
	uint16_t gprocs[GROUPS_PER_MACHINE];
	uint32_t gqueues[GROUPS_PER_MACHINE];
};

struct CFieldConfig {
	const char *name;
	char type;
	int size;
	CValue dval;
	int flags;
};

struct CKeyHash{
	int keyHashEnable;
	int keyHashLeftBegin; 	/* buff 的左起始位置 */
	int keyHashRightBegin;  /* buff 的右起始位置 */
	key_hash_interface keyHashFunction;
};

struct CDbConfig {
	CConfig *cfgObj;
	char *dbName;
	char *dbFormat;
	char *tblName;
	char *tblFormat;

	int dstype;	/* data-source type: default is mysql*/
	int checkTable;
	unsigned int dbDiv;
	unsigned int dbMod;
	unsigned int tblMod;
	unsigned int tblDiv;
	int fieldCnt;
	int keyFieldCnt;
	int idxFieldCnt;
	int machineCnt;
	int procs;  //all machine procs total
	int dbMax; //max db index
	char depoly; //0:none 1:multiple db 2:multiple table 3:both

	struct CKeyHash keyHashConfig;

	int slaveGuard;
	int autoinc;
	int lastacc;
	int lastmod;
	int lastcmod;
    int compressflag;
	int ordIns;
	char *ordSql;
	struct CFieldConfig *field;
	struct CMachineConfig *mach;

	static struct CDbConfig* Load(const char *file);
	static struct CDbConfig* Load(CConfig *);
	void Destroy(void);
	void DumpDbConfig(FILE *);
	void SetHelperPath(int);
	CTableDefinition *BuildTableDefinition(void);
	int LoadKeyHash(CConfig *);
private:
	int ParseDbConfig(CConfig *);
};


#endif
