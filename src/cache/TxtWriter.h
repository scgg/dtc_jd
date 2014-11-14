#ifndef __TXT_WRITER_H
#define __TXT_WRITER_H

#include <stdio.h>
#include <stdlib.h>

#include "field.h"
#include "buffer.h"
#include "writer_interface.h"

class cTxtWriter: public cWriterInterface
{
	private:
        //操作的文件指针
		FILE* pstOutFile;
        //用于缓存转义的字符串或binary
		class buffer stEscape;
		char szErrMsg[200];
	
	protected:
		int WriteValue(int iFieldId, int iFieldType, const CValue& stValue);
		
	public:
		cTxtWriter();
		~cTxtWriter();
		
		const char* err_msg(void){ return szErrMsg; }
		
		int Open(const char* szDataFile, const char* szMode="w");
		int Close();
		int write_row(const CRowValue& row);
		int commit_node(){ return 0; }
		int rollback_node(){ return 0; }
		int full(){ return 0; }
};

#endif
