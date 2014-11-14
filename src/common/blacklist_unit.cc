#include  <blacklist_unit.h>

TTC_USING_NAMESPACE

CBlackListUnit::CBlackListUnit(CTimerList * t) :
	timer(t) 
{
}

CBlackListUnit::~CBlackListUnit()
{
}

void CBlackListUnit::TimerNotify(void)
{
	log_debug("sched blacklist-expired task");

	/* expire all expired eslot */
	try_expired_blacklist();

	CBlackList::dump_all_blslot();
	
	/* set timer agagin */
	AttachTimer(timer);
	
	return;
}
