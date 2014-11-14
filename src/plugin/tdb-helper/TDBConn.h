#ifndef __TTC_TDB_CONN_H
#define __TTC_TDB_CONN_H

#include <vector>
#include <string>
#include <stdint.h>
#include <namespace.h>
#include <tdb_handle.h>

TTC_BEGIN_NAMESPACE

typedef struct _TDBConfig
{
	uint32_t t_id;
	uint32_t c_id;
	uint32_t freeze;
	uint32_t timeout;
	uint32_t retryflushtime;
	std::string route;
	std::vector<std::string> hands;
}TDBConfig;

class CTDBConn
{
	public:
		CTDBConn();
		~CTDBConn();

		int Open(TDBConfig* );	
		int Set(std::string& key, std::string& value);
		int Get(std::string& key, std::string& value);
		int Del(std::string& key);
		int Close(void);

		/* TDB只支持单行记录，这个接口留作以后扩展 */
		int AffectedRows(void) { return 1;}

		const char *Error(void) const 
		{
			return _errmsg;
		}
		const int ErrorNo(void) const
		{
			return _errNo;
		}

	private:
		tdb::api::CTdbHandle* _tdbHandle;
		TDBConfig*  _tdbConfig;
		char        _errmsg[256];
		int	    _errNo;
};

TTC_END_NAMESPACE

#endif
