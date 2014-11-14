#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbconfig.h"
#include "DBConn.h"
#include "DBReader.h"

cDBReader::cDBReader()
{
	pstDBConn = NULL;
	pstDBConfig = NULL;
	pstTabDef = NULL;
	iCurMachIdx = 0;
	iCurDBIdx = 0;
	iCurTabIdx = -1;
	iCurRowIdx = 0;
	iTabRowNum = 0;
	memset(szErrMsg, 0, sizeof(szErrMsg));
}

cDBReader::~cDBReader()
{
	if(pstDBConn != NULL)
		delete []pstDBConn;
	if(pstDBConfig != NULL)
		delete pstDBConfig;
	if(pstTabDef != NULL)
		delete pstTabDef;
}

int cDBReader::Init(const char* szTabCfgFile)
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
		
		for(int j=0; j<pstDBConfig->mach[i].dbCnt; j++){
			int iDBIdx = pstDBConfig->mach[i].dbIdx[j] % pstDBConfig->dbMax;
			pshMachineMap[iDBIdx] = i;
		}
	}
	
	// Æ´SQLÓï¾ä
	iPrintErr = 0;
	SQLPrintf(stSQLTemplate, "SELECT ");
	for(i=0; i<=pstTabDef->NumFields(); i++){
		if(i==0)
			SQLPrintf(stSQLTemplate, "%s", pstTabDef->FieldName(i));
		else if(!pstTabDef->IsVolatile(i))
			// Discard field always be Volatile
			SQLPrintf(stSQLTemplate, ",%s", pstTabDef->FieldName(i));
	}
	SQLPrintf(stSQLTemplate, " FROM %s ORDER BY %s", pstDBConfig->tblFormat, pstTabDef->FieldName(0));
	if(pstDBConfig->ordSql != NULL && pstDBConfig->ordSql[0] != 0){
		char* p=NULL;
		if(GetOrderField(pstDBConfig->ordSql, p) != 0)
			return(-5);
		SQLPrintf(stSQLTemplate, ",%s ", p);
		free(p);
	}
	
	if(iPrintErr != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "printf error: %m");
		return(-6);
	}
	
	return(0);
}

int cDBReader::SQLPrintf(buffer& stBuf, const char *Format, ...) 
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

#define SKIP_SPACE(p) while(*p && isspace(*p))p++
#define SKIP_NAME(p) while(*p && *p!=',' && !isspace(*p))p++
#define SKIP_NONSPACE(p) while(*p && !isspace(*p))p++

int cDBReader::GetOrderField(const char* szOrderSQL, char*& pszField)
{
	char* szUpperCase;
	char* p;
	const char* begin;
	const char* end;
	
	szUpperCase = strdup(szOrderSQL);
	if(szUpperCase == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "strdup error: %m");
		return(-1);
	}
	p = szUpperCase;
	while(*p){
		*p = toupper(*p);
		p++;
	}
	if((p=strstr(szUpperCase, "ORDER")) == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "invalid order sql-'order' not found");
		free(szUpperCase);
		return(-2);
	}
	p+=6;
	if((p=strstr(szUpperCase, "BY")) == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "invalid order sql-'by' not found");
		free(szUpperCase);
		return(-3);
	}
	p+=2;

	SKIP_SPACE(p);
	begin = szOrderSQL+(p-szUpperCase); // order field begin
	end = begin;
	
	while(*p){
		SKIP_NAME(p); // skip field name
		end = szOrderSQL+(p-szUpperCase);
		SKIP_SPACE(p); // skip space
		if(*p){
			if(*p != ','){
				if(strncmp(p, "DESC", 4)!=0 && strncmp(p, "ASC", 3)!=0){
					break;
				}
				SKIP_NAME(p); // skip desc|asc
				end = szOrderSQL+(p-szUpperCase);
				SKIP_SPACE(p); // skip space
				if(*p && *p!=',')
					break;
			}
			
			p++; // skip ','
			SKIP_SPACE(p); // skip space
		}
	}
	free(szUpperCase);
	
	pszField = (char*)calloc(end-begin+1, sizeof(char));
	if(pszField == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "calloc error: %m");
		return(-4);
	}
	
	if(end > begin)
		strncpy(pszField, begin, end-begin);
	
	return(0);
}

int cDBReader::ReadTable()
{
	int iRet;

CHECK_MACH:	
	if(iCurDBIdx >= pstDBConfig->mach[iCurMachIdx].dbCnt){ // ÇÐ»»DB»úÆ÷
		iCurMachIdx++;
		if(iCurMachIdx >= pstDBConfig->machineCnt){
			snprintf(szErrMsg, sizeof(szErrMsg), "end of database");
			return(-1);
		}
		iCurDBIdx = 0;
		iCurTabIdx = 0;
		iCurRowIdx = 0;
	}
	
	if(iCurTabIdx >= (int)pstDBConfig->tblMod){
		iCurDBIdx++;
		iCurTabIdx = 0;
		iCurRowIdx = 0;
		goto CHECK_MACH;
	}
	
	// query db now
	char szDBName[256];
	if(pstDBConfig->depoly & 1)
		snprintf(szDBName, sizeof(szDBName), pstDBConfig->dbFormat, pstDBConfig->mach[iCurMachIdx].dbIdx[iCurDBIdx]);
	else
		snprintf(szDBName, sizeof(szDBName), "%s", pstDBConfig->dbFormat);
	
	stSQL.clear();
	iPrintErr = 0;
	if(pstDBConfig->depoly & 2)
		SQLPrintf(stSQL, stSQLTemplate.c_str(), iCurTabIdx);
	else
		SQLPrintf(stSQL, "%s", stSQLTemplate.c_str());
	if(iPrintErr != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "printf error: %m");
		return(-2);
	}
	
	pstDBConn[iCurMachIdx].FreeResult(); // clear last result
	
	iRet = pstDBConn[iCurMachIdx].Query(szDBName, stSQL.c_str());
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "db error: %s", pstDBConn[iCurMachIdx].GetErrMsg());
		return(-3);
	}
	
	// use result
	iRet = pstDBConn[iCurMachIdx].UseResult();
	if(iRet != 0)
	{
		snprintf(szErrMsg, sizeof(szErrMsg), "use result error: %s", pstDBConn[iCurMachIdx].GetErrMsg());
		return(-3);
	}
	iTabRowNum = pstDBConn[iCurMachIdx].ResNum;
	
	iCurRowIdx = 0;
	
	return(0);
}

int cDBReader::SetDefaultValue(int FieldType, CValue& Value)
{
	switch(FieldType){
		case DField::Signed:
			Value.s64 = 0;
			break;
			
		case DField::Unsigned:
			Value.u64 = 0;
			break;
			
		case DField::Float:
			Value.flt = 0.0;
			break;
			
		case DField::String:
			Value.str.len = 0;
			Value.str.ptr = 0; 
			break;
				
		case DField::Binary:
			Value.bin.len = 0;
			Value.bin.ptr = 0;
			break;
			
		default:
			Value.s64 = 0;		
	};
	
	return(0);
}

int cDBReader::Str2Value(char* Str, int fieldid, int FieldType, CValue& Value)
{
	if(Str == NULL)
    {
		SetDefaultValue(FieldType, Value);
		return(0);
	}
	
	switch(FieldType)
    {
		case DField::Signed:
			errno = 0;
			Value.s64 = strtoll(Str, NULL, 10);
			if(errno != 0)
				return(-1);
			break;
			
		case DField::Unsigned:
			errno = 0;
			Value.u64 = strtoull(Str, NULL, 10);
			if(errno != 0)
				return(-1);
			break;
			
		case DField::Float:
			errno = 0;
			Value.flt = strtod(Str, NULL);
			if(errno != 0)
				return(-1);
			break;
			
		case DField::String:
			Value.str.len = pulFieldResultLen[fieldid];
			Value.str.ptr = Str;
			break;
				
		case DField::Binary:
			Value.bin.len = pulFieldResultLen[fieldid];
			Value.bin.ptr = Str;
			break;
			
		default:
			snprintf(szErrMsg, sizeof(szErrMsg), "field[%d] type[%d] invalid.", fieldid, FieldType);
            break;
	}
	
	return(0);
}

int cDBReader::SaveRow(CRowValue& Row)
{
	int i, iRet,j;
	
	Row.DefaultValue();
	for(i=0, j=0; i<=pstTabDef->NumFields(); i++){ 
		// Discard field always be Volatile
		if(pstTabDef->IsVolatile(i)) {
			continue;
		}
		iRet = Str2Value(pstDBConn[iCurMachIdx].Row[j], j, pstTabDef->FieldType(j), Row[i]);
		j++;

		if(iRet != 0){
			snprintf(szErrMsg, sizeof(szErrMsg), "string[%s] conver to value[%d] error: %d, %m", pstDBConn[iCurMachIdx].Row[i], pstTabDef->FieldType(i), iRet);
			return(-1);
		}
	}

	return(0);	
}


int cDBReader::read_row(CRowValue& row)
{
	int iRet;
	
	while(iCurRowIdx >= iTabRowNum){
		pstDBConn[iCurMachIdx].FreeResult();
		iCurTabIdx++;
		iRet = ReadTable();
		if(iRet != 0)
			return(-1);
	}
	
	iRet = pstDBConn[iCurMachIdx].FetchRow();
	if(iRet != 0){
		snprintf(szErrMsg, sizeof(szErrMsg), "fetch row error: %s", pstDBConn[iCurMachIdx].GetErrMsg());
		pstDBConn[iCurMachIdx].FreeResult();
		return(-2);
	}
	iCurRowIdx++;
	
	//get field value length for the row
	pulFieldResultLen = 0;
	pulFieldResultLen = pstDBConn[iCurMachIdx].getLengths();
		
	iRet = SaveRow(row);
	if(iRet != 0)
		return(-3);
		
	return(0);
}


