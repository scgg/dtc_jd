#ifndef __WRITER_INTERFACE_H
#define __WRITER_INTERFACE_H

#include "field.h"

class cWriterInterface{
	public:
		cWriterInterface(){}
		virtual ~cWriterInterface(){}

		virtual const char* err_msg()=0;
		
		virtual int begin_write(){ return 0; }
		virtual int write_row(const CRowValue& row)=0;
		virtual int commit_node()=0;
		virtual int rollback_node()=0;
		virtual int full()=0;
};

#endif
