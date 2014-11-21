#ifndef KEY_ROUTE_H__
#define KEY_ROUTE_H__

#include <map>
#include <string>
#include <vector>

#include "request_base.h"
#include "task_request.h"
#include "consistent_hash_selector.h"
#include "parse_cluster_config.h"

class CFileBackedKeySet;

class CKeyRoute : public CTaskDispatcher<CTaskRequest>
{
public:
    void BindRemoteHelper(CTaskDispatcher<CTaskRequest> *r)
	{
			m_remoteOutput.BindDispatcher(r);
	}
    void BindCache(CTaskDispatcher<CTaskRequest> *c)
	{
			m_cacheOutput.BindDispatcher(c);
	}
    int KeyMigrated(const char *key);

    void Init(const std::vector<ClusterConfig::ClusterNode> &nodes);
    void TaskNotify(CTaskRequest *t);
    int LoadNodeStateIfAny();

    CKeyRoute(CPollThread *owner, int keyFormat);
private:
    CRequestOutput<CTaskRequest> m_cacheOutput;
    CRequestOutput<CTaskRequest> m_remoteOutput;
    int m_keyFormat;

    void ProcessReload(CTaskRequest *t);
    void ProcessNodeStateChange(CTaskRequest *t);
    void ProcessChangeNodeAddress(CTaskRequest *t);
	void ProcessMigrate(CTaskRequest *t);
    void ProcessGetClusterState(CTaskRequest *t);
    bool AcceptKey(const std::string &node, const char *key, int len);

    const char *NameToAddr(const std::string &node)
    {
        return m_serverMigrationState[node].addr.c_str();
    }

    std::string KeyListFileName(const std::string &name)
    {
        return "../data/" + name + ".migrated";
    }

    std::string SelectNode(const char *key);

    bool MigrationInprogress();
    void SaveStateToFile();

    enum MigrationState
    {
        MS_NOT_STARTED,
        MS_MIGRATING,
        MS_MIGRATE_READONLY,
        MS_COMPLETED,
    };

    struct ServerMigrationState
    {
        std::string addr;
        int state;
        CFileBackedKeySet *migrated;

        ServerMigrationState() : state(MS_NOT_STARTED), migrated(NULL) {}
    };

    typedef std::map<std::string, ServerMigrationState> MigrationStateMap;
    MigrationStateMap m_serverMigrationState;
    std::string m_selfName;
    CConsistentHashSelector m_selector;
};

#endif
