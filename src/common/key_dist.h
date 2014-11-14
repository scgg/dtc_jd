#include "value.h"

class CKeyDist {
public:
	CKeyDist() {}
	virtual ~CKeyDist() = 0;
	virtual unsigned int HashKey(const CValue *) = 0;
	virtual int Key2DBId(const CValue *) = 0;
	virtual int Key2Table(const CValue *, char *, int) = 0;
};

