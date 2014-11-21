#include <strings.h>
#include "TDBConn.h"
#include "tdb_handle.h"
#include "namespace.h"
#include "log.h"

TTC_USING_NAMESPACE

CTDBConn::CTDBConn():
	_tdbHandle(0),
	_tdbConfig(0),
	_errNo(0)
{
	bzero(_errmsg, sizeof(_errmsg));
}

CTDBConn::~CTDBConn()
{
	Close();
}

int CTDBConn::Open(TDBConfig *config)
{
	if(!config || config->hands.size() < 1) {
		snprintf(_errmsg, sizeof(_errmsg), 
				"not found tdb server");
		return -1;
	}

	if(config->t_id <= 0 || config->c_id <= 0) {
		snprintf(_errmsg, sizeof(_errmsg),
				"t_id=%u, c_id=%u is invalid",
				config->t_id, config->c_id);
		return -1;
	}

	tdb::api::CTdbHandle *_tdbHandle = new tdb::api::CTdbHandle();
	if(!_tdbHandle) {
		snprintf(_errmsg, sizeof(_errmsg),
				"create tdbhandle obj failed");
		return -1;
	}

	if(config->freeze)
		_tdbHandle->SetFreezeLinkSecs(config->freeze);

	if(config->timeout)
		_tdbHandle->SetTime(config->timeout, config->timeout);

    for(unsigned i = 0; i < config->hands.size(); ++i)
    {
        _tdbHandle->AddAddr(config->hands[i]);
		log_debug("connected to tdb server: %s", config->hands[i].c_str());
	}

	_tdbConfig = config;
	return 0;
}

int CTDBConn::Close(void)
{
	delete _tdbHandle;
	_tdbHandle = NULL;
	return 0;
}

int CTDBConn::Set(std::string& key, std::string& value)
{
	return _errNo = _tdbHandle->Set(_tdbConfig->t_id, _tdbConfig->c_id, key, value);
}

int CTDBConn::Get(std::string& key, std::string& value)
{
	return _errNo = _tdbHandle->Get(_tdbConfig->t_id, _tdbConfig->c_id, key, value);
}

int CTDBConn::Del(std::string& key)
{
	return _errNo = _tdbHandle->Del(_tdbConfig->t_id, _tdbConfig->c_id, key);
}
