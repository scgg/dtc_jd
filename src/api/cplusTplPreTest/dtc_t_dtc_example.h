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
	class DTCt_dtc_example;

	const int RequestGet 		= 4;
	const int RequestPurge 		= 5;
	const int RequestInsert 	= 6;
	const int RequestUpdate 	= 7;
	const int RequestDelete 	= 8;

	const string tablename = "t_dtc_example";
	const string dtcKey = "uid";
	const string ip = "1.1.1.2";
	const string port = "10901";
	const string token = "0000090184d9cfc2f395ce883a41d7ffc1bbcf4e";
	const int timeout = 5;
	const string fieldcfg[][4] = {
			{"uid", "1", "-2147483648", "2147483647"},
			{"name", "4", "0", "100"},
			{"city", "4", "0", "100"},
			{"descr", "5", "0", "255"},
			{"age", "1", "-2147483648", "2147483647"},
			{"salary", "1", "-2147483648", "2147483647"},
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

	class DTCt_dtc_example{
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
		DTCt_dtc_example();
		~DTCt_dtc_example();
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

