/* vim: set sw=4 ai: */
#include <unistd.h>
#include <errno.h>
#include <endian.h>
#include <byteswap.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include "task_base.h"
#include "decode.h"
#include "protocol.h"
#include "cache_error.h"
#include "log.h"

int CTask::AppendResult(CResultSet *rs)
{
	if(rs == NULL){
		SetTotalRows(0);
		return 0;
	}
	rs->Rewind();
	if(AllRows() && (CountOnly() || !InRange(rs->TotalRows(), 0)))
	{
		SetTotalRows(rs->TotalRows());
	}
	else
	{
		for(int i=0; i<rs->TotalRows(); i++) {
			const CRowValue *r = rs->FetchRow();
			if(r==NULL)
				return rs->ErrorNum();
			if(CompareRow(*r)) {
				int rv = resultWriter->AppendRow(*r);
				if(rv < 0) return rv;
				if(AllRows() && ResultFull()) {
					SetTotalRows(rs->TotalRows());
					break;
				}
			}
		}
	}
	return 0;
}

/* newman: pool */
/* all use prepareResult interface */
int CTask::PassAllResult(CResultSet *rs)
{
	PrepareResult(0, 0);
    /*
	try {
		resultWriter = new CResultPacket(fieldList, 0, 0);
	} catch(int err) {
		SetError(err, "CResultPacket()", NULL);
		return err;
	}
*/	
	rs->Rewind();
	if(CountOnly())
	{
		SetTotalRows(rs->TotalRows());
	}
	else
	{
		for(int i=0; i<rs->TotalRows(); i++) {
			int rv;
			const CRowValue *r = rs->FetchRow();
			if(r==NULL)
			{
				rv = rs->ErrorNum();
				SetError(rv, "FetchRow()", NULL);
				return rv;
			}
			rv = resultWriter->AppendRow(*r);
			if(rv < 0) {
				SetError(rv, "AppendRow()", NULL);
				return rv;
			}
		}
	}
	resultWriter->SetTotalRows( (unsigned int)(resultInfo.TotalRows()) );
	
	return 0;
}

int CTask::MergeResult(const CTask& task)
{
	int ret = task.resultInfo.AffectedRows();
	
	if(ret > 0) {
		resultInfo.SetAffectedRows(ret + resultInfo.AffectedRows());
	}

	if(task.resultWriter == NULL)
		return 0;

	if(resultWriter == NULL){
		if((ret = PrepareResultNoLimit()) != 0){
			return ret;
		}
	}
	return resultWriter->MergeNoLimit(task.resultWriter);
}

/* newman: pool, the only entry to create resultWriter */
int CTask::PrepareResult(int st, int ct) {
	int err;

	if(resultWriterReseted && resultWriter)
	    return 0;

	if(resultWriter)
	{
	    err = resultWriter->Set(fieldList, st, ct);
	    if(err < 0){
		SetError(-EC_TASKPOOL, "CResultPacket()", NULL);
		return err;
	    }
	}else
	{
	    try {
		resultWriter = new CResultPacket(fieldList, st, ct);
	    } catch(int err) {
		SetError(err, "CResultPacket()", NULL);
		return err;
	    }
	}

	resultWriterReseted = 1;

	return 0;
}
