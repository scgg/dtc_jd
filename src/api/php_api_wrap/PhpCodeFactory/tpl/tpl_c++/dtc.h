#ifndef __DTC_TEMPLATE_API_H__
#define __DTC_TEMPLATE_API_H__

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "ttcapi.h"

using namespace std;
using namespace TTC;

namespace DTC{
	class DTC{TableName};

	const int RequestGet 		= 4;
	const int RequestPurge 		= 5;
	const int RequestInsert 	= 6;
	const int RequestUpdate 	= 7;
	const int RequestDelete 	= 8;

	const string tablename = "{TableName}";
	const string dtcKey = "{Dtckey}";
	const string ip = "{IP}";
	const string port = "{Port}";
	const string token = "{Token}";
	const int timeout = 5;
	const string fieldcfg[][4] = {
			<!-- BEGIN FIELD1 -->
			{"{FIELD1Name}", "{FIELD1Type}", "{minsize}", "{maxsize}"},
			<!-- END FIELD1 -->
	};

	enum DTCErrorCode{
		E_SET_TTIMEOUT_FAIL 	= 20000,
		E_INVALID_KEY_TYPE		= 20001,
		E_INVALID_FIELD_TYPE 	= 20002,
		E_INVALID_FIELD_VALUE   = 20003,
		E_INVALID_KEY_VALUE 	= 20004,
		E_KEY_MISMATCH 			= 20005,
		E_EMPTY_FIELDNAME 		= 20006,
		E_INVALID_FIELDNAME 	= 20007,
	};
	// None=0, Signed=1, Unsigned=2, Float=3, String=4, Binary=5
	enum DTCKeyType{
		None		= 0,
		Signed		= 1,
		Unsigned	= 2,
		Float		= 3,
		String 		= 4,
		Binary		= 5,
	};

	class Field{
	public:
		string fieldName;
		int fieldType;				// None=0, Signed=1, Unsigned=2, Float=3, String=4, Binary=5
		string fieldValue;
	};

	class DTC{TableName}{
	public:
		int errCode;
		int dtcCode;
		string errMsg;
		string errFrom;
		int affectRows;
	private:
		int keyType;
		int keyIndex;
		string dtckey;
		Server dtcServer;

	public:
		DTC{TableName}();
		~DTC{TableName}();
		bool purge(string key);
		bool delet(string key);
		bool get(string key, vector<Field>& need, vector<Field>& resultSet);
		bool update(string key, vector<Field>& fieldList);
		bool insert(string key, vector<Field>& fieldList);
		int getAffectRows();
	private:
		void init();
		void clearErr();
		bool checkKey(string key);
		bool check(vector<Field>& fieldList);
		int GetFlag(string fieldName);
	};

}

#endif

