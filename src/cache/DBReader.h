#ifndef __DB_READER_H
#define __DB_READER_H

#include "dbconfig.h"
#include "buffer.h"
#include "field.h"
#include "reader_interface.h"

class CDBConn;

class cDBReader: public cReaderInterface
{
	private:
		CDBConn* pstDBConn;
		CDbConfig* pstDBConfig;
		CTableDefinition* pstTabDef;
		short* pshMachineMap;
		int iCurMachIdx;
		int	iCurDBIdx;
		int iCurTabIdx;
		int iCurRowIdx;
		int iTabRowNum;
		
		int iPrintErr;
		buffer stSQLTemplate;
		buffer stSQL;
		unsigned long* pulFieldResultLen;
		
		char szErrMsg[200];
	
	protected:
		int GetOrderField(const char* szOrderSQL, char*& pszField);
		int SQLPrintf(buffer& stBuf, const char *Format, ...);
		int ReadTable();
		int SetDefaultValue(int FieldType, CValue& Value);
		int Str2Value(char* Str, int fieldid, int FieldType, CValue& Value);
		int SaveRow(CRowValue& Row);
		
	public:
		cDBReader();
		~cDBReader();
		
		int Init(const char* szTabCfgFile);
		const char* err_msg(void){ return szErrMsg; }
		const CDbConfig* GetDBConf(){ return pstDBConfig; }
		const CTableDefinition* GetTabDef(){ return pstTabDef; }
		
		int read_row(CRowValue& row);
		int end();
		int CurTabRowNum(){ return iTabRowNum; }
		int CurRowIdx() { return iCurRowIdx; }
		void FetchTable(){ iCurRowIdx = 0; iTabRowNum = 0; }
};

inline int cDBReader::end()
{
	return (iCurMachIdx >= pstDBConfig->machineCnt);
}

#endif
