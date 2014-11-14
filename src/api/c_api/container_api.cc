
#include <stdio.h>
#include <dlfcn.h>

#include "container.h"
#include "version.h"
#include "ttcint.h"

typedef IInternalService *(*QueryInternalServiceFunctionType)(const char *name, const char *instance);

IInternalService *QueryInternalService(const char *name, const char *instance)
{
	QueryInternalServiceFunctionType entry = NULL;
	entry = (QueryInternalServiceFunctionType)dlsym(RTLD_DEFAULT, "_QueryInternalService");
	if(entry == NULL)
		return NULL;
	return entry(name, instance);
}

static inline int fieldtype2keytype(int t)
{
	switch(t) {
		case DField::Signed:
		case DField::Unsigned:
			return DField::Signed;

		case DField::String:
		case DField::Binary:
			return DField::String;
	}

	return DField::None;
}

void NCServer::CheckInternalService(void)
{
	if(NCResultInternal::VerifyClass()==0)
		return;

	IInternalService *inst = QueryInternalService("ttcd", this->tablename);

	// not inside ttcd or tablename not found
	if(inst == NULL)
		return;

	// version mismatch, internal service is unusable
	const char *version = inst->QueryVersionString();
	if(version==NULL || strcasecmp(version_detail, version) != 0)
		return;

	// cast to TTC service
	ITTCService *inst1 = static_cast<ITTCService *>(inst);

	CTableDefinition *tdef = inst1->QueryTableDefinition();

	// verify tablename etc
	if(tdef->IsSameTable(tablename)==0)
		return;

	// verify and save key type
	int kt = fieldtype2keytype(tdef->KeyType());
	if(kt == DField::None)
		// bad key type
		return;

	if(keytype == DField::None) {
		keytype = kt;
	} else if(keytype != kt) {
		badkey = 1;
		errstr = "Key Type Mismatch";
		return;
	}
	
	if(keyinfo.KeyFields()!=0) {
		// FIXME: checking keyinfo

		// ZAP key info, use ordered name from server
		keyinfo.Clear();
	}
	// add NCKeyInfo
	for(int i=0; i<tdef->KeyFields(); i++) {
		kt = fieldtype2keytype(tdef->FieldType(i));
		if(kt == DField::None)
			// bad key type
			return;
		keyinfo.AddKey(tdef->FieldName(i), kt);
	}

	// OK, save it.
	// old tdef always NULL, because tablename didn't set, Server didn't complete
	this->tdef = tdef;
	this->admin_tdef = inst1->QueryAdminTableDefinition();
	// useless here, internal CTableDefinition don't maintent a usage count
	tdef->INC();
	this->iservice = inst1;
	this->completed = 1;
	if(GetAddress() && iservice->MatchListeningPorts(GetAddress(), NULL)) {
		executor = iservice->QueryTaskExecutor();
	}
}

