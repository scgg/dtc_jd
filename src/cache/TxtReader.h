#ifndef __TXT_READER_H
#define __TXT_READER_H

#include <stdio.h>
#include <stdlib.h>

#include "field.h"
#include "reader_interface.h"

class cTxtReader: public cReaderInterface
{
	private:
		FILE* pstInFile;
		ssize_t tLineLen;
		size_t tLineBufSize;
		char* pszLine;
		char szErrMsg[200];
	
	protected:
		int SetDefaultValue(int FieldType, CValue& Value);
		int Str2Value(char** ppszStr, int iFieldId, int iFieldType, CValue& stValue);
		int SaveRow(CRowValue& Row);
		
	public:
		cTxtReader();
		~cTxtReader();
		
		const char* err_msg(void){ return szErrMsg; }
		
		int Open(const char* szDataFile);
		int Close();
		int read_row(CRowValue& row);
		int end();
};

inline int cTxtReader::end()
{
	if(pstInFile != NULL)
		return feof(pstInFile);
	return(1);
}

#endif
