
#include "decoder_base.h"
#include "poll_thread.h"

CDecoderUnit::CDecoderUnit(CPollThread *owner, int idletimeout)
{
	this->owner = owner;
	this->idleTime = idletimeout;
	this->idleList = owner->GetTimerList(idletimeout);
}

CDecoderUnit::~CDecoderUnit()
{
}
