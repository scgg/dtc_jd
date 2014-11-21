#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dbconfig.h"
#include "TxtWriter.h"
#include "TxtEscape.h"

cTxtWriter::cTxtWriter()
{
	pstOutFile = NULL;
	memset(szErrMsg, 0, sizeof(szErrMsg));
}

cTxtWriter::~cTxtWriter()
{
	Close();
}

//打开文件
//返回值：
//-1，出错
//0，ok
int cTxtWriter::Open(const char* szDataFile, const char* szMode)
{
	pstOutFile = fopen64(szDataFile, szMode);
	if(pstOutFile == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "open file[%s] error: %m", szDataFile);
		return(-1);
	}

	return(0);
}

//关闭文件
int cTxtWriter::Close()
{
	if(pstOutFile != NULL)
		fclose(pstOutFile);
	pstOutFile = NULL;
	
	return(0);
}


//将stValue里的值根据iFieldType指定的类型写到文件中
//返回值:
//<0,出错
//=0，ok
int cTxtWriter::WriteValue(int iFieldId, int iFieldType, const CValue& stValue)
{

	switch(iFieldType){
		case DField::Signed:
			fprintf(pstOutFile, "%lld", (long long)stValue.s64);
			break;
			
		case DField::Unsigned:
			fprintf(pstOutFile, "%llu", (unsigned long long)stValue.u64);
			break;	
			
		case DField::Float:
			fprintf(pstOutFile, "%f", stValue.flt);
			break;
			
		case DField::String:
			stEscape.clear();
			if(stEscape.expand(stValue.str.len*2+1) < 0){
				snprintf(szErrMsg, sizeof(szErrMsg), "expend buffer to size:%u error: %m", stValue.str.len*2+1);
				return(-1);
			}
			cTxtEscape::EscapeStr(stEscape.c_str(), stValue.str.ptr, stValue.str.len);
			fprintf(pstOutFile, "%s", stEscape.c_str());
			break;
			
		case DField::Binary:
			stEscape.clear();
			if(stEscape.expand(stValue.str.len*2+1) < 0){
				snprintf(szErrMsg, sizeof(szErrMsg), "expend buffer to size:%u error: %m", stValue.str.len*2+1);
				return(-1);
			}
			cTxtEscape::EscapeBin(stEscape.c_str(), stValue.str.len, stValue.str.ptr);
			fprintf(pstOutFile, "%s", stEscape.c_str());
			break;

		default:
			;				
	};
		
	return 0;
}

//将一行数据写到文件中，跳过discard属性的字段
//返回值：
//<0,出错
//=0，ok
int cTxtWriter::write_row(const CRowValue& row)
{
	int iRet;
	int i;
	
	if(pstOutFile == NULL){
		snprintf(szErrMsg, sizeof(szErrMsg), "output file not open yet");
		return(-1);
	}
	
	for(i=0; i<=row.TableDefinition()->NumFields(); i++){ 
		if(row.TableDefinition()->IsDiscard(i)) {
			continue;
		}
		if(i > 0) {
			fprintf(pstOutFile, "\t");
		}
		iRet = WriteValue(i, row.TableDefinition()->FieldType(i), row[i]);
		if(iRet != 0){
			snprintf(szErrMsg, sizeof(szErrMsg), "write value[%d-type:%d] error: %d, %m", i, row.TableDefinition()->FieldType(i), iRet);
			return(-2);
		}
	}
	fprintf(pstOutFile, "\n");
		
	return(0);
}


