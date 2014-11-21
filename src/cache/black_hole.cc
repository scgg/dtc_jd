#include <black_hole.h>

CBlackHole::~CBlackHole(void)
{
}

void CBlackHole::TaskNotify(CTaskRequest *cur)
{
#if 0
	switch(cur->RequestCode()){
		case DRequest::Get:
			break;
			
		case DRequest::Insert: // TableDef->HasAutoIncrement() must be false
			cur->resultInfo.SetAffectedRows(1);
			break;
		
		case DRequest::Update:
		case DRequest::Delete:
		case DRequest::Purge:
		case DRequest::Replace:
		default:
			cur->resultInfo.SetAffectedRows(1);
			break;
	}
	
#else
	// preset AffectedRows==0 is obsoleted
	// use BlackHole flag instead
	cur->MarkAsBlackHole();
#endif
	cur->ReplyNotify();
}

