#ifndef __TTC_TDB_PROCESS_H
#define __TTC_TDB_PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "CommProcess.h"
#include "TDBConn.h"

	
TTC_BEGIN_NAMESPACE

class CTDBConn;
class CTDBHelper : public CCommHelper, private TDBConfig
{
	public:
		CTDBHelper();
		virtual ~CTDBHelper();

		// 服务需要实现的几个函数
		virtual int GobalInit(void);
		virtual int Init(void);

	protected:
		// 服务需要实现的几个处理函数
		virtual int ProcessGet();
		virtual int ProcessInsert();
		virtual int ProcessDelete();
		virtual int ProcessUpdate();
		virtual int ProcessReplace();
	
	private:
		int MakeKeyString(std::string& key);
		int MakeValueString(const CValue*, int, std::string&);

		int DecodeRow(CRowValue&, std::string&);
		int EncodeRow(std::string&);

	private:
		CTDBConn  _tdbConn[1];
};

TTC_END_NAMESPACE

#endif
