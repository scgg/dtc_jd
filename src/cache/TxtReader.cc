#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbconfig.h"
#include "TxtReader.h"
#include "TxtEscape.h"

cTxtReader::cTxtReader()
{
	pstInFile = NULL;
	tLineBufSize = 0;
	pszLine = NULL;
	memset(szErrMsg, 0, sizeof(szErrMsg));
}

cTxtReader::~cTxtReader()
{
	if(pszLine != NULL)
		free(pszLine);
	Close();
}

int cTxtReader::SetDefaultValue(int FieldType, CValue& Value)
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

int cTxtReader::Str2Value(char** ppszStr, int iFieldId, int iFieldType, CValue& stValue)
{
//	while(isblank(**ppszStr))
//		(*ppszStr)++;
	if(**ppszStr == '\t')
		(*ppszStr)++;
	if(**ppszStr == 0 || **ppszStr=='\t' || **ppszStr=='\n'){
		SetDefaultValue(iFieldType, stValue);
		return(0);
	}
	
	char* p=*ppszStr;
	switch(iFieldType)
    {
		case DField::Signed:
			errno = 0;
			stValue.s64 = strtoll(p, ppszStr, 0);
			if(errno != 0 || p==*ppszStr)
				return(-1);
			break;
			
		case DField::Unsigned:
			errno = 0;
			stValue.u64 = strtoull(p, ppszStr, 0);
			if(errno != 0 || p==*ppszStr)
				return(-1);
			break;
			
		case DField::Float:
			errno = 0;
			stValue.flt = strtod(p, ppszStr);
			if(errno != 0 || p==*ppszStr)
				return(-1);
			break;
			
		case DField::String:
			stValue.str.ptr = *ppszStr;
			stValue.str.len = cTxtEscape::UnescapeStr(ppszStr, '\n');
//			if(**ppszStr == '\t')
//				(*ppszStr)++;
			break;
				
		case DField::Binary:
			stValue.bin.ptr = *ppszStr;
			stValue.bin.len = cTxtEscape::UnescapeBin(ppszStr, '\n');
			if(stValue.bin.len < 0){
				snprintf(szErrMsg, sizeof(szErrMsg), "field[%d] binary unescape[%s] error", iFieldId, stValue.bin.ptr);
				return(-1);
			}
//			if(**ppszStr == '\t')
//				(*ppszStr)++;
			break;
			
		default:
			snprintf(szErrMsg, sizeof(szErrMsg), "field[%d] type[%d] invalid.", iFieldId, iFieldType);
            break;
	}
	
	return(0);
}

int cTxtReader::Open(const char* szDataFile)
{
	pstInFile = fopen64(szDataFile, "r");
	if(pstInFile == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "open file[%s] error: %m", szDataFile);
		return(-1);
	}

	return(0);
}

int cTxtReader::Close()
{
	if(pstInFile != NULL)
		fclose(pstInFile);
	pstInFile = NULL;
	
	return(0);
}

int cTxtReader::SaveRow(CRowValue& Row)
{
	int i, iRet;
	char* pszRow;
	
	pszRow = pszLine;
	for(i=0; i<=Row.TableDefinition()->NumFields(); i++){ 
		if(Row.TableDefinition()->IsDiscard(i)) {
			continue;
		}
		iRet = Str2Value(&pszRow, i, Row.TableDefinition()->FieldType(i), Row[i]);
		if(iRet != 0){
			snprintf(szErrMsg, sizeof(szErrMsg), "string[%s] conver to value-type[%d] error: %d, %m", pszRow, Row.TableDefinition()->FieldType(i), iRet);
			return(-1);
		}
	}

	return(0);	
}

int cTxtReader::read_row(CRowValue& row)
{
	int iRet;
	
	if(pstInFile == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "input file not open yet");
		return(-1);
	}
	
	tLineLen = getline(&pszLine, &tLineBufSize, pstInFile);
	if(tLineLen <= 0){
		if(feof(pstInFile))
			snprintf(szErrMsg, sizeof(szErrMsg), "end of file");
		else
			snprintf(szErrMsg, sizeof(szErrMsg), "read file error: %m");
		return(-2);
	}
	
	iRet = SaveRow(row);
	if(iRet != 0)
		return(-3);
		
	return(0);
}


