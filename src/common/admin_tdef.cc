#include "log.h"
#include "admin_tdef.h"
#include "protocol.h"
#include "dbconfig.h"

static struct CFieldConfig HBTabField[] = {
				{"type", DField::Unsigned, 4, CValue::Make(0), 0},
				{"flag", DField::Unsigned, 1, CValue::Make(0), 0},
				{"key", DField::Binary, 255, CValue::Make(0), 0},
				{"value", DField::Binary, MAXPACKETSIZE, CValue::Make(0), 0},
			};

CTableDefinition* BuildHotBackupTable(void)
{
	CTableDefinition *tdef = new CTableDefinition (4);
	tdef->SetTableName ("@HOT_BACKUP");
	struct CFieldConfig* field = HBTabField;
	int field_cnt = sizeof(HBTabField)/sizeof(HBTabField[0]);
	
	tdef->SetAdminTable();
	for (int i = 0; i < field_cnt; i++) {
		if (tdef->AddField (i, field[i].name, field[i].type, field[i].size) != 0)
		{
			log_error ("AddField failed, name=%s, size=%d, type=%d", 
				field[i].name, field[i].size, field[i].type);
			delete tdef;
			return NULL;
		}
		tdef->SetDefaultValue(i, &field[i].dval);
	}
	
	if(tdef->SetKeyFields (1) < 0) {
		log_error("Table key size %d too large, must <= 255",
			tdef->KeyFormat() > 0 ? tdef->KeyFormat() : -tdef->KeyFormat());
		delete tdef;
		return NULL;
	}
	tdef->BuildInfoCache ();
	return tdef;
}

