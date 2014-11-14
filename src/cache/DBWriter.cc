#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbconfig.h"
#include "DBConn.h"
#include "DBWriter.h"

cDBWriter::cDBWriter()
{
	pstDBConn = NULL;
	pstDBConfig = NULL;
	pstTabDef = NULL;
}

cDBWriter::~cDBWriter()
{
	if(pstDBConn != NULL)
		delete []pstDBConn;
	if(pstDBConfig != NULL)
		delete pstDBConfig;
	if(pstTabDef != NULL)
		delete pstTabDef;
}

int cDBWriter::Init(const char* szTabCfgFile)
{
	int i;
	
	pstDBConfig = CDbConfig::Load(szTabCfgFile);
	if(pstDBConfig == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "load table configuire file error");
		return(-1);
	}
	
	pstTabDef = pstDBConfig->BuildTableDefinition();
	if(pstTabDef == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "build table definition error");
		return(-2);
	}
	
	pstDBConn = new CDBConn[pstDBConfig->machineCnt];
	if(pstDBConn == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "new CDBConn error: %m");
		return(-3);
	}
	pshMachineMap = (short*)malloc(sizeof(short)*pstDBConfig->dbMax);
	if(pshMachineMap == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "malloc machine-map error: %m");
		return(-4);
	}
	for(i=0; i<pstDBConfig->dbMax; i++)
		pshMachineMap[i] = -1;
		
	DBHost stDBAddr;
	for(i=0; i<pstDBConfig->machineCnt; i++){
		memset(&stDBAddr, 0, sizeof(stDBAddr));
		strncpy(stDBAddr.Host, pstDBConfig->mach[i].role[0].addr, sizeof(stDBAddr.Host)-1);
		stDBAddr.Port = 0;
		strncpy(stDBAddr.User, pstDBConfig->mach[i].role[0].user, sizeof(stDBAddr.User)-1);
		strncpy(stDBAddr.Password, pstDBConfig->mach[i].role[0].pass, sizeof(stDBAddr.Password)-1);
		pstDBConn[i].Config(&stDBAddr);
		pstDBConn[i].Open();
		
		for(int j=0; j<pstDBConfig->mach[i].dbCnt; j++){
			int iDBIdx = pstDBConfig->mach[i].dbIdx[j] % pstDBConfig->dbMax;
			pshMachineMap[iDBIdx] = i;
		}
	}
	
	// 拼SQL语句
	iPrintErr = 0;
	SQLPrintf(stSQLTemplate, "%%s INTO %%s (");
	for(i=0; i<=pstTabDef->NumFields(); i++){
		// Discard fields always volatile
		if(pstTabDef->IsVolatile(i)) {
			continue;
		}
		if(i==0)
			SQLPrintf(stSQLTemplate, "%s", pstTabDef->FieldName(i));
		else
			SQLPrintf(stSQLTemplate, ",%s", pstTabDef->FieldName(i));
	}
	SQLPrintf(stSQLTemplate, ") VALUES ");
	
	if(iPrintErr != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "printf error: %m");
		return(-6);
	}
	
	return(0);
}

int cDBWriter::SQLPrintf(buffer& stBuf, const char *Format, ...) 
{
	va_list Arg;
	int Len;
  
	va_start (Arg, Format);
	Len = stBuf.vbprintf (Format, Arg);
	va_end (Arg); 
	if(Len < 0){
		iPrintErr = 1;
		return(-1);
	}
	
	return(0);
} 

void cDBWriter::InitTableName(const CValue *Key, int FieldType)
{
	int dbid = 0, tableid = 0;
	int64_t n;
	double f;

	if(Key != NULL && pstDBConfig->depoly != 0)
	{
		switch(FieldType)
		{
			case DField::Signed:
				if(Key->s64 >= 0)
					n = Key->s64;
#ifndef LONG_LONG_MIN
# if __WORDSIZE == 64
#  define LONG_LONG_MIN  (-9223372036854775807L-1)
# else
#  define LONG_LONG_MIN  (-2147483647-1)
# endif
#endif
				else if(Key->s64 == LONG_LONG_MIN)
					n = 0;
				else
					n = 0 - Key->s64;

				dbid = (n/pstDBConfig->dbDiv)%pstDBConfig->dbMod;
				tableid = (n/pstDBConfig->tblDiv)%pstDBConfig->tblMod;
				break;

			case DField::Unsigned:
				dbid = (Key->u64/pstDBConfig->dbDiv)%pstDBConfig->dbMod;
				tableid = (Key->u64/pstDBConfig->tblDiv)%pstDBConfig->tblMod;
				break;

			case DField::Float:
				if(Key->flt >= 0)
					f = Key->flt;
				else
					f = 0 - Key->flt;

				dbid = ((int)(f/pstDBConfig->dbDiv))%pstDBConfig->dbMod;
				tableid = ((int)(f/pstDBConfig->tblDiv))%pstDBConfig->tblMod;
				break;
				// String/Binary hasn't db/table distribution
		}
	}

	iCurDBIdx = dbid;
	snprintf(DBName, sizeof(DBName), pstDBConfig->dbFormat, dbid);
	snprintf(TableName, sizeof(TableName), pstDBConfig->tblFormat, tableid);
}

inline int cDBWriter::Value2Str(const CValue* Value, int iFieldType)
{
	if(Value==NULL) {
		SQLPrintf(stSQL, "NULL");
	} else
	switch(iFieldType){
		case DField::Signed:
			SQLPrintf(stSQL, "%lld", (long long)Value->s64);
			break;
			
		case DField::Unsigned:
			SQLPrintf(stSQL, "%llu", (unsigned long long)Value->u64);
			break;	
			
		case DField::Float:
			SQLPrintf(stSQL, "'%f'", Value->flt);
			break;
			
		case DField::String:
		case DField::Binary:
			if(stSQL.append('\'') < 0)
				iPrintErr = -1;
			if(!Value->str.IsEmpty())
			{
				stEscape.clear();
				if(stEscape.expand(Value->str.len*2+1) < 0)
				{
					iPrintErr = -1;
					return(0);
				}
				pstDBConn[pshMachineMap[iCurDBIdx]].EscapeString(stEscape.c_str(), Value->str.ptr, Value->str.len); // 先对字符串进行escape
				if(stSQL.append(stEscape.c_str()) < 0)
					iPrintErr = -1;
			}
			if(stSQL.append('\'')<0)
				iPrintErr = -1;
			break;

		default:
			;				
	};
		
	return 0;
}


int cDBWriter::ModifyRow(int iOp, unsigned int uiRowCnt, const CRowValue* row)
{
	int iRet;
	char szOp[24];
	
	// query db now
	if(iOp == DB_WRITE_REPLACE)
		snprintf(szOp, sizeof(szOp), "REPLACE");
	else
		snprintf(szOp, sizeof(szOp), "INSERT");
	InitTableName(row->FieldValue(0), pstTabDef->FieldType(0));
	if(pshMachineMap[iCurDBIdx] < 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "key not belong to this server");
		return(-1);
	}
	
	stSQL.clear();
	iPrintErr = 0;
	SQLPrintf(stSQL, stSQLTemplate.c_str(), szOp, TableName);
	for(unsigned int i=0; i<uiRowCnt; i++){
		if(stSQL.append('(') < 0){
			iPrintErr = -1;
			break;
		}
		for(int j=0; j<=pstTabDef->NumFields(); j++){
			// Discard fields always volatile
			if(pstTabDef->IsVolatile(i)) {
				continue;
			}
			if(j > 0){
				if(stSQL.append(',') < 0){
					iPrintErr = -1;
					break;
				}
			}
			Value2Str((row+i)->FieldValue(j), pstTabDef->FieldType(j));
		}
		if(stSQL.append(") ") < 0){
			iPrintErr = -1;
			break;
		}
		if(iPrintErr != 0)
			break;
	}
	
	if(iPrintErr != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "printf error: %m");
		return(-2);
	}
	
	iRet = pstDBConn[pshMachineMap[iCurDBIdx]].Query(DBName, stSQL.c_str());
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "db error: %s", pstDBConn[pshMachineMap[iCurDBIdx]].GetErrMsg());
		return(-3);
	}
	
	return(0);
}

int cDBWriter::InsertRow(unsigned int uiRowCnt, const CRowValue* row)
{
	return ModifyRow(DB_WRITE_INSERT, uiRowCnt, row);
}

int cDBWriter::ReplaceRow(unsigned int uiRowCnt, const CRowValue* row)
{
	return ModifyRow(DB_WRITE_REPLACE, uiRowCnt, row);
}

