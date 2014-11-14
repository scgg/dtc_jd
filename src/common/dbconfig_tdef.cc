#include "log.h"
#include "dbconfig.h"
#include "tabledef.h"

CTableDefinition * CDbConfig::BuildTableDefinition(void)
{
	CTableDefinition *tdef = new CTableDefinition (fieldCnt);
	tdef->SetTableName (tblName);
	for (int i = 0; i < fieldCnt; i++) {
		if (tdef->AddField (i, field[i].name, field[i].type, field[i].size) != 0)
		{
			log_error ("AddField failed, name=%s, size=%d, type=%d", 
				field[i].name, field[i].size, field[i].type);
			delete tdef;
			return NULL;
		}
		tdef->SetDefaultValue(i, &field[i].dval);
		if((field[i].flags & DB_FIELD_FLAGS_READONLY))
			tdef->MarkAsReadOnly(i);
		if((field[i].flags & DB_FIELD_FLAGS_VOLATILE))
			tdef->MarkAsVolatile(i);
		if((field[i].flags & DB_FIELD_FLAGS_DISCARD))
			tdef->MarkAsDiscard(i);
		if((field[i].flags & DB_FIELD_FLAGS_UNIQ))
			tdef->MarkUniqField(i);
		if((field[i].flags & DB_FIELD_FLAGS_DESC_ORDER)){
			tdef->MarkOrderDesc(i);
			if(tdef->IsDescOrder(i))
				log_debug("success set field[%d] desc order", i);
			else
				log_debug("set field[%d] desc roder fail", i);
		}
	}
	if(autoinc >= 0) tdef->SetAutoIncrement(autoinc);
	if(lastacc >= 0) tdef->SetLastacc(lastacc);
	if(lastmod >= 0) tdef->SetLastmod(lastmod);
	if(lastcmod >= 0) tdef->SetLastcmod(lastcmod);
	if(compressflag >= 0) tdef->SetCompressFlag(compressflag);

	if(tdef->SetKeyFields (keyFieldCnt) < 0) {
		log_error("Table key size %d too large, must <= 255",
			tdef->KeyFormat() > 0 ? tdef->KeyFormat() : -tdef->KeyFormat());
		delete tdef;
		return NULL;
	}
	tdef->SetIndexFields(idxFieldCnt);
	tdef->BuildInfoCache ();
	return tdef;
}

