#ifndef __DB_WRITER_H
#define __DB_WRITER_H

#include "dbconfig.h"
#include "buffer.h"
#include "field.h"
#include "writer_interface.h"

class CDBConn;

class cDBWriter: public cWriterInterface
{
	private:
		CDBConn* pstDBConn;
		CDbConfig* pstDBConfig;
		CTableDefinition* pstTabDef;
		short* pshMachineMap;
		
		int iPrintErr;
		class buffer stSQLTemplate;
		class buffer stEscape;
		class buffer stSQL;
		int iCurDBIdx;
		char DBName[64];
		char TableName[64];
		char szErrMsg[200];
		
		enum{
			DB_WRITE_INSERT=0,
			DB_WRITE_REPLACE
		};
		
	protected:
		int SQLPrintf(buffer& stBuf, const char *Format, ...);
		void InitTableName(const CValue *Key, int FieldType);
		int Value2Str(const CValue* Value, int iFieldType);
		
		int ModifyRow(int iOp, unsigned int uiRowCnt, const CRowValue* row);
		
	public:
		cDBWriter();
		~cDBWriter();
		
		int Init(const char* szTabCfgFile);
		const char* err_msg(void){ return szErrMsg; }
		const CTableDefinition* GetTabDef(){ return pstTabDef; }
		
		// 如果uiRowCnt > 1，调用者需要确保所有row按照field0分表都是在同一张表里
		int InsertRow(unsigned int uiRowCnt, const CRowValue* row);
		int ReplaceRow(unsigned int uiRowCnt, const CRowValue* row);
		
		int write_row(const CRowValue& row){ return ReplaceRow(1, &row); }
		int commit_node(){ return 0; }
		int rollback_node(){ return 0; }
		int full(){ return 0; }
};


#endif
