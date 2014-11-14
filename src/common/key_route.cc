#include <cstdio>
#include <cassert>

#include "key_route.h"
#include "file_backed_key_set.h"
#include "admin_tdef.h"
#include "field.h"

CKeyRoute::CKeyRoute(CPollThread *owner, int keyFormat) : CTaskDispatcher<CTaskRequest>(owner), m_cacheOutput(owner),
    m_remoteOutput(owner), m_keyFormat(keyFormat)
{
}

void CKeyRoute::TaskNotify(CTaskRequest *t)
{
    if(t->RequestCode() == DRequest::SvrAdmin)
    {
	    log_debug("AdminCode:%d",t->requestInfo.AdminCode());
        switch(t->requestInfo.AdminCode())
        {
        case DRequest::ServerAdminCmd::ReloadClusterNodeList:
            ProcessReload(t);
            return;
        case DRequest::ServerAdminCmd::SetClusterNodeState:
            ProcessNodeStateChange(t);
            return;
        case DRequest::ServerAdminCmd::ChangeNodeAddress:
            ProcessChangeNodeAddress(t);
            return;
        case DRequest::ServerAdminCmd::GetClusterState:
            ProcessGetClusterState(t);
            return;
		case DRequest::ServerAdminCmd::Migrate:
			ProcessMigrate(t);
			return;
        default:
            m_cacheOutput.TaskNotify(t);
            return;
        }
    }

    if(t->RequestCode() == DRequest::Nop)
    {
        //just pass through
        m_cacheOutput.TaskNotify(t);
        return;
    }

    if (t->PackedKey() == NULL)
    {
	    t->SetError(-EC_BAD_OPERATOR, "Key Route", "Batch Request Not Supported");
	    t->ReplyNotify();
	    return;
    }
    std::string selected = SelectNode(t->PackedKey());
    log_debug("route to:%s self is:%s",selected.c_str(),m_selfName.c_str());
    const char *packedKey = t->PackedKey();
    int keyLen = m_keyFormat > 0 ? m_keyFormat : (*(unsigned char *)packedKey) + 1;

    if(selected == m_selfName)
    {
        m_cacheOutput.TaskNotify(t);
        return;
    }

    if(AcceptKey(selected, packedKey, keyLen))
    {
        t->SetRemoteAddr(NameToAddr(selected));
        t->MarkAsPassThru();
        m_remoteOutput.TaskNotify(t);
        return;
    }
    else
    {
        MigrationStateMap::iterator i = m_serverMigrationState.find(selected);
        if(i->second.state == MS_MIGRATE_READONLY)
        {
            if(t->RequestCode() != DRequest::Get)
            {
                t->SetError(-EC_SERVER_READONLY, "Key Route", "migrate readonly");
                t->ReplyNotify();
                return;
            }
        }


        m_cacheOutput.TaskNotify(t);
    }
}

//Migrate命令需要在keyRoute中设置后端forward的TTC地址
//具体处理在cache_admin中
void CKeyRoute::ProcessMigrate(CTaskRequest *t)
{
    std::string selected = SelectNode(t->PackedKey());

	log_debug("migrate to %s", selected.c_str());
    if(selected == m_selfName)
    {
        const CFieldValue *ui = t->RequestOperation();
        if(ui && ui->FieldValue(0) &&
                (ui->FieldValue(0)->s64 & 0xFF) == CMigrate::FROM_SERVER)
        {
            m_cacheOutput.TaskNotify(t);
            return;
        }

        log_info("key belongs to self,no migrate operate");
        t->SetError(-EC_STATE_ERROR, "Key Route", "key belongs to self");
        t->ReplyNotify();
        return;
    }

    MigrationStateMap::iterator iter = m_serverMigrationState.find(selected);
    if(iter == m_serverMigrationState.end())
    {
        t->SetError(-EC_STATE_ERROR, "Key Route", 
                "internal error: node not in map!");
        t->ReplyNotify();
        log_error("internal error: %s not in serverMigrationState", selected.c_str());
		return;
    }

    if(iter->second.state != MS_MIGRATING &&
        iter->second.state != MS_MIGRATE_READONLY)
    {
        t->SetError(-EC_STATE_ERROR, "Key Route", "state not migratable");
        t->ReplyNotify();
        log_error("%s not migrating while key migrated!", selected.c_str());
        return ;
    }
    t->SetRemoteAddr(NameToAddr(selected));
    m_cacheOutput.TaskNotify(t);
}

std::string CKeyRoute::SelectNode(const char *packedKey)
{
    int keyLen = m_keyFormat > 0 ? m_keyFormat : (*(unsigned char *)packedKey) + 1;
    std::string selected;
    //agent don't know how long the key is, so use u64 always
    if(m_keyFormat == 4)
    {
        uint64_t v = *(uint32_t *)packedKey;
        return m_selector.Select((const char *)&v, sizeof(uint64_t));
    }

    return m_selector.Select(packedKey, keyLen);
}

int CKeyRoute::KeyMigrated(const char *key)
{
    std::string selected = SelectNode(key);
    MigrationStateMap::iterator iter = m_serverMigrationState.find(selected);
    if(iter == m_serverMigrationState.end())
    {
        log_crit("internal error: %s not in serverMigrationState", selected.c_str());
        return -1;
    }

    if(iter->second.state != MS_MIGRATING &&
        iter->second.state != MS_MIGRATE_READONLY)
    {
        log_crit("%s not migrating while key migrated!", selected.c_str());
        return -1;
    }

    return iter->second.migrated->Insert(key);
}

bool CKeyRoute::AcceptKey(const std::string &node, const char *key, int len)
{
    MigrationStateMap::iterator iter = m_serverMigrationState.find(node);
    if(iter == m_serverMigrationState.end())
    {
        //this should never happen!
        log_crit("can't find %s in state map!", node.c_str());
        //forward it either way
        return true;
    }

    if(iter->second.state == MS_NOT_STARTED)
        return false;
    
    if(iter->second.state == MS_COMPLETED)
        return true;

    return iter->second.migrated->Contains(key);
}

void CKeyRoute::ProcessReload(CTaskRequest *t)
{
    //在迁移未完成前不允许切换表
    if(MigrationInprogress())
    {
        t->SetError(-EC_STATE_ERROR, "Key Route", 
                "Reload not possible while migration in-progress");
        t->ReplyNotify();
        return;
    }

    if(!t->RequestOperation())
    {
        t->SetError(-EC_DATA_NEEDED, "Key Route", "table needed");
        t->ReplyNotify();
        return;
    }

    CRowValue row(t->TableDefinition());
    t->UpdateRow(row);

    //the table should be placed in the last field
    log_debug("get new table, len %d", row[3].bin.len);
    std::vector<ClusterConfig::ClusterNode> nodes;
    if(!ClusterConfig::ParseClusterConfig(&nodes, row[3].bin.ptr, row[3].bin.len))
    {
        t->SetError(-EC_BAD_INVALID_FIELD, "Key Route", "invalid table");
        t->ReplyNotify();
        return;
    }
	if (!ClusterConfig::SaveClusterConfig(&nodes))
	{
			t->SetError(-EC_SERVER_ERROR, "Key Route", "wirte config to file error");
			t->ReplyNotify();
			return;
	}

    m_serverMigrationState.clear();
    m_selector.Clear();

    log_debug("got new table");
    for(std::vector<ClusterConfig::ClusterNode>::iterator i = nodes.begin();
            i != nodes.end(); ++i)
    {
        log_debug("\tnode %s : %s, self? %s", i->name.c_str(),
                i->addr.c_str(), i->self ? "yes" : "no");
        ServerMigrationState s;
        s.addr = i->addr;
        s.state = i->self ? MS_COMPLETED : MS_NOT_STARTED;
        s.migrated = NULL;
        m_serverMigrationState[i->name] = s;
        if(i->self)
            m_selfName = i->name;
        m_selector.AddNode(i->name.c_str());
    }

    SaveStateToFile();

    t->PrepareResultNoLimit();
    t->ReplyNotify();
}

static const char *migrateStateStr[] = {
    "MIGRATION NOT STARTED",
    "MIGRATING",
    "MIGRATION READONLY",
    "MIGRATION COMPLETED",
};

void CKeyRoute::ProcessGetClusterState(CTaskRequest *t)
{
    t->PrepareResultNoLimit();
    for(MigrationStateMap::iterator iter = m_serverMigrationState.begin();
            iter != m_serverMigrationState.end(); ++iter)
    {
        std::string result = iter->first;
        if(m_selfName == iter->first)
            result += " * ";
        result += "\t";
        result += iter->second.addr;
        result += "\t\t";

        if(iter->second.state < MS_NOT_STARTED || iter->second.state > MS_COMPLETED)
            result += "!!!INVALID STATE!!!";
        else
            result += migrateStateStr[iter->second.state];

        CRowValue row(t->TableDefinition());
        row[3].Set(result.c_str(), result.size());
        t->AppendRow(row);
    }

    t->ReplyNotify();
}

void CKeyRoute::ProcessChangeNodeAddress(CTaskRequest *t)
{
    if(!t->RequestOperation())
    {
        t->SetError(-EC_DATA_NEEDED, "Key Route", "name/address needed");
        t->ReplyNotify();
        return;
    }

    CRowValue row(t->TableDefinition());
    t->UpdateRow(row);
    std::string name(row[2].bin.ptr, row[2].bin.len);
    std::string addr(row[3].bin.ptr, row[3].bin.len);

    MigrationStateMap::iterator iter = m_serverMigrationState.find(name);
    if(iter == m_serverMigrationState.end())
    {
        t->SetError(-EC_BAD_RAW_DATA, "Key Route", "invalid node name");
        t->ReplyNotify();
        return;
    }
	if(!ClusterConfig::ChangeNodeAddress(name,addr))
	{
			t->SetError(-EC_SERVER_ERROR, "Key Route", "change node address in config file error");
			t->ReplyNotify();
			return;
	}

    iter->second.addr = addr;
    t->PrepareResultNoLimit();
    t->ReplyNotify();
}


void CKeyRoute::ProcessNodeStateChange(CTaskRequest *t)
{
    if(!t->RequestOperation())
    {
        t->SetError(-EC_DATA_NEEDED, "Key Route", "node name needed.");
        t->ReplyNotify();
        return;
    }

    CRowValue row(t->TableDefinition());
    t->UpdateRow(row);
    std::string name(row[2].bin.ptr, row[2].bin.len);
    int newState = row[1].u64;
    MigrationStateMap::iterator iter = m_serverMigrationState.find(name);
    if(newState > MS_COMPLETED || iter == m_serverMigrationState.end())
    {
        t->SetError(-EC_BAD_RAW_DATA, "Key Route", "invalid name/state");
        t->ReplyNotify();
        return;
    }

    if(iter->second.state == newState)
    {
        //nothing changed
        t->PrepareResultNoLimit();
        t->ReplyNotify();
        return;
    }

    if(name == m_selfName)
    {
        t->SetError(-EC_BAD_RAW_DATA, "Key Route", "can't change self's state");
        t->ReplyNotify();
        return;
    }

    switch((iter->second.state << 4) | newState)
    {
    case (MS_NOT_STARTED << 4) | MS_MIGRATE_READONLY:
        log_warning("node[%s] state changed from NOT_STARTED to MIGRATE_READONLY", name.c_str());
        //fall through
    case (MS_NOT_STARTED << 4) | MS_MIGRATING:
        iter->second.migrated = new CFileBackedKeySet(KeyListFileName(name).c_str(), t->TableDefinition()->KeyFormat());
        if(iter->second.migrated->Open() < 0)
        {
            delete iter->second.migrated;
            iter->second.migrated = NULL;
            t->SetError(-EC_SERVER_ERROR, "Key Route",  "open file backed key set failed");
            t->ReplyNotify();
            return;
        }
        iter->second.state = newState;
        break;

    case (MS_NOT_STARTED << 4) | MS_COMPLETED:
        //when we add a node to the cluster, only data belongs to the new node need to be migrated
        //other nodes are untouched. 
        //this is the case.
        log_info("node[%s] state changed from NOT_STARTED to COMLETED", name.c_str());
        iter->second.state = newState;
        break;

    case (MS_MIGRATING << 4) | MS_NOT_STARTED:
    case (MS_MIGRATE_READONLY << 4) | MS_NOT_STARTED:
        //XXX: should we allow these changes?
        log_warning("node[%s] state changed from %s to NOT_STARTED!",
                name.c_str(),
                iter->second.state == MS_MIGRATING ? "MIGRATING" : "MIGRATE_READONLY");
        iter->second.migrated->Clear();
        delete iter->second.migrated;
        iter->second.state = newState;
        break;

    case (MS_MIGRATE_READONLY << 4) | MS_MIGRATING:
        log_warning("node[%s] state changed from MIGRATE_READONLY to MIGRATING",
                name.c_str());
        //fall through
    case (MS_MIGRATING << 4) | MS_MIGRATE_READONLY:
        //normal change
        iter->second.state = newState;
        break;

    case (MS_MIGRATING << 4) | MS_COMPLETED:
        log_warning("node[%s] state changed from MIGRATING to COMPLETED",
                name.c_str());
        //fall through
    case(MS_MIGRATE_READONLY << 4) | MS_COMPLETED:
        iter->second.migrated->Clear();
        delete iter->second.migrated;
        iter->second.state = newState;
        break;

    case (MS_COMPLETED << 4) | MS_NOT_STARTED:
        log_warning("node[%s] state changed from COMPLETED to NOT_STARTED",
                name.c_str());
        iter->second.state = newState;
        break;

    case (MS_COMPLETED << 4) | MS_MIGRATING:
    case (MS_COMPLETED << 4) | MS_MIGRATE_READONLY:
        //XXX: should we allow these changes?
        log_warning("node[%s] state changed from COMPLATED to %s!",
                name.c_str(),
                newState == MS_MIGRATING ? "MIGRATING" : "MIGRATE_READONLY");
        iter->second.migrated = new CFileBackedKeySet(KeyListFileName(name).c_str(), t->TableDefinition()->KeyFormat());
        if(iter->second.migrated->Open() < 0)
        {
            delete iter->second.migrated;
            iter->second.migrated = NULL;
            t->SetError(-EC_SERVER_ERROR, "Key Route", "open file backed key set failed\n");
            t->ReplyNotify();
            return;
        }
        iter->second.state = newState;
        break;

    default:
        log_crit("invalid state/new state value : %d --> %d",
                iter->second.state, newState);
        t->SetError(-EC_BAD_RAW_DATA, "Key Route", "invalid state");
        t->ReplyNotify();
        return;
    }

    //state changed, save it to file
    SaveStateToFile();
    t->PrepareResultNoLimit();
    t->ReplyNotify();
}

bool CKeyRoute::MigrationInprogress()
{
    for(MigrationStateMap::iterator iter = m_serverMigrationState.begin();
            iter != m_serverMigrationState.end(); ++iter)
    {
        if(iter->second.state == MS_MIGRATING ||
                iter->second.state == MS_MIGRATE_READONLY)
            return true;
    }

    return false;
}

static const char *state_file_name = "../data/cluster.stat";

void CKeyRoute::SaveStateToFile()
{
    FILE *f = fopen(state_file_name, "wb");
    if(!f)
    {
        log_crit("open %s for write failed, %m", state_file_name);
        return;
    }

    int n = m_serverMigrationState.size();
    fwrite(&n, sizeof(n), 1, f);
    for(MigrationStateMap::iterator iter = m_serverMigrationState.begin();
            iter != m_serverMigrationState.end(); ++iter)
    {
        int len = iter->first.size();
        fwrite(&len, sizeof(len), 1, f);
        fwrite(iter->first.c_str(), 1, len, f);
        fwrite(&iter->second.state, sizeof(iter->second.state), 1, f);
    }

    fclose(f);
}

void CKeyRoute::Init(const std::vector<ClusterConfig::ClusterNode> &nodes)
{
    for(std::vector<ClusterConfig::ClusterNode>::const_iterator i = nodes.begin();
            i != nodes.end(); ++i)
    {
        log_info("\tnode %s : %s, self? %s", i->name.c_str(),
                i->addr.c_str(), i->self ? "yes" : "no");
        ServerMigrationState s;
        s.addr = i->addr;
        s.state = i->self ? MS_COMPLETED : MS_NOT_STARTED;
        s.migrated = NULL;
        m_serverMigrationState[i->name] = s;
        if(i->self)
            m_selfName = i->name;
        m_selector.AddNode(i->name.c_str());
    }
}

int CKeyRoute::LoadNodeStateIfAny()
{
    FILE *f = fopen(state_file_name, "rb");
    if(!f)  //it is ok if the state file is not there
        return 0;

    log_info("begin loading stat file ...");

    int n = 0;
    assert(fread(&n, sizeof(n), 1, f) == 1);
    if(n != (int)m_serverMigrationState.size())
    {
        log_crit("state file and config file mismatch!");
        fclose(f);
        return -1;
    }

    char buf[256];  //name can't exceeds 255
    for(int i = 0; i < n; ++i)
    {
        int len = 0;
        assert(fread(&len, sizeof(len), 1, f) == 1);
        if(len == 0 || len > (int)sizeof(buf) - 1)
        {
            log_crit("invalid state file!");
            fclose(f);
            return -1;
        }
        assert((int)fread(buf, 1, len, f) == len);
        buf[len] = 0;
        int state = 0;
        assert(fread(&state, sizeof(state), 1, f) == 1);
        if(state < MS_NOT_STARTED || state > MS_COMPLETED)
        {
            log_crit("invalid state in state file!");
            fclose(f);
            return -1;
        }

        MigrationStateMap::iterator iter = m_serverMigrationState.find(buf);
        if(iter == m_serverMigrationState.end())
        {
            log_crit("name %s in state file doesn't exist in config!", buf);
            fclose(f);
            return -1;
        }

        iter->second.state = state;
        log_info("%s %s", iter->first.c_str(), migrateStateStr[state]);
        if(state == MS_MIGRATING || state == MS_MIGRATE_READONLY)
        {
            iter->second.migrated = new CFileBackedKeySet(
                    KeyListFileName(iter->first).c_str(), m_keyFormat);
            if(iter->second.migrated->Load() < 0)
            {
                log_crit("load %s migrated key list failed", buf);
                fclose(f);
                return -1;
            }
        }
    }

    //we should got EOF
    assert(fread(buf, 1, 1, f) == 0);
    fclose(f);

    log_info("load state file completed.");
    return 0;
}





