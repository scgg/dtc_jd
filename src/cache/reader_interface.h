#ifndef __READER_INTERFACE_H
#define __READER_INTERFACE_H

#include "field.h"

class cReaderInterface{
	public:
		cReaderInterface(){}
		virtual ~cReaderInterface(){}

		virtual const char* err_msg()=0;
		virtual int begin_read(){ return 0; }
		virtual int read_row(CRowValue& row)=0;
		virtual int end()=0;
};

#endif
