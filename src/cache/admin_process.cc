/************************************************************
Description:    ��װ�˳�ʼ���ڴ�cache�Լ�����task����ĸ��ֲ���      
Version:         TTC 3.0
Function List:   
1.  cache_open()	��cache�ڴ�
2.  cache_process_request()	����task����
3. cache_process_reply() 		����helper�ķ���
4. cache_process_error()		task����Ĵ���
History:        
Paul    2008.07.01     3.0         build this moudle  
 ***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <endian.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "packet.h"
#include "log.h"
#include "cache_process.h"
#include "cache_flush.h"
#include "mysql_error.h"
#include "sys_malloc.h"
#include "data_chunk.h"
#include "raw_data_process.h"
#include "admin_tdef.h"
#include "key_route.h"

TTC_USING_NAMESPACE

extern CTableDefinition *gTableDef[];
extern int targetNewHash;
extern int hashChanging;
extern CKeyRoute * keyRoute;
#if __WORDSIZE == 64
# define UINT64FMT_T "%lu"
#else
# define UINT64FMT_T "%llu"
#endif

TCacheResult CCacheProcess::cache_process_admin(CTaskRequest &Task)
{
	 log_debug("CCacheProcess::cache_process_admin AdminCode is %d ",Task.requestInfo.AdminCode()  );
	if (Task.requestInfo.AdminCode() == DRequest::ServerAdminCmd::QueryServerInfo
			|| Task.requestInfo.AdminCode() == DRequest::ServerAdminCmd::LogoutHB
			|| Task.requestInfo.AdminCode() == DRequest::ServerAdminCmd::GetUpdateKey)
	{
		if(hbFeature == NULL){ // �ȱ�������δ����
			Task.SetError(-EBADRQC,CACHE_SVC,"hot-backup not active yet");
			return CACHE_PROCESS_ERROR;
		}
	}

	switch(Task.requestInfo.AdminCode()){
		case DRequest::ServerAdminCmd::QueryServerInfo:
			return cache_query_serverinfo(Task);

		case DRequest::ServerAdminCmd::RegisterHB:
			return cache_register_hb(Task);

		case DRequest::ServerAdminCmd::LogoutHB:
			return cache_logout_hb(Task);

		case DRequest::ServerAdminCmd::GetKeyList:
			return cache_get_key_list(Task);

		case DRequest::ServerAdminCmd::GetUpdateKey:
			return cache_get_update_key(Task);

		case DRequest::ServerAdminCmd::GetRawData:
			return cache_get_raw_data(Task);

		case DRequest::ServerAdminCmd::ReplaceRawData:
			return cache_replace_raw_data(Task);

		case DRequest::ServerAdminCmd::AdjustLRU:
			return cache_adjust_lru(Task);

		case DRequest::ServerAdminCmd::VerifyHBT:
			return cache_verify_hbt(Task);

		case DRequest::ServerAdminCmd::GetHBTime:
			return cache_get_hbt(Task);

		case DRequest::ServerAdminCmd::NodeHandleChange:
			return cache_nodehandlechange(Task);

		case DRequest::ServerAdminCmd::Migrate:
			return cache_migrate(Task);

		default:
			Task.SetError(-EBADRQC,CACHE_SVC,"invalid admin cmd code from client");
			log_notice("invalid admin cmd code[%d] from client", Task.requestInfo.AdminCode());
			break;
	}

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_register_hb(CTaskRequest &Task)
{
	if(hbFeature == NULL){ // �����ڴ滹û�м����ȱ�����
		NEW(CHBFeature, hbFeature);
		if(hbFeature == NULL){
			log_error("new hot-backup feature error: %m");
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "new hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
		int iRet = hbFeature->Init(time(NULL));
		if(iRet == -ENOMEM){
			CNode stNode;
			if(Cache.TryPurgeSize(1, stNode) == 0)
				iRet = hbFeature->Init(time(NULL));
		}
		if(iRet != 0){
			log_error("init hot-backup feature error: %d", iRet);
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "init hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
		iRet = Cache.AddFeature(HOT_BACKUP, hbFeature->Handle());
		if(iRet != 0){
			log_error("add hot-backup feature error: %d", iRet);
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "add hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
	}
	if(hbFeature->MasterUptime() == 0)
		hbFeature->MasterUptime() = time(NULL);

	//�������key��־
	hbLog.EnableLog();

	//����lru������־
	lruLog.EnableLog();

	//ע��һ��Journal ID
	CJournalID client_jid = Task.versionInfo.HotBackupID();
	CJournalID master_jid = hbLog.GetWriterJID();
	int64_t hb_timestamp = hbFeature->MasterUptime();

	log_notice("hb register, client[serial=%u, offset=%u], master[serial=%u, offset=%u]", 
			client_jid.serial, client_jid.offset, master_jid.serial, master_jid.offset);

	Task.versionInfo.SetMasterHBTimestamp(hb_timestamp);
	Task.versionInfo.SetSlaveHBTimestamp(hbFeature->SlaveUptime());
	//ȫ�¿ͻ���, ȫ��
	if(client_jid.Zero())
	{
		log_info("full-sync stage.");

		Task.versionInfo.SetHotBackupID((uint64_t)master_jid);
		Task.SetError(-EC_FULL_SYNC_STAGE, "register", "full-sync stage");
		return CACHE_PROCESS_ERROR;
	}
	else
	{
		//����
		if (hbLog.Seek(client_jid) == 0)
		{
			log_info("inc-sync stage.");

			Task.versionInfo.SetHotBackupID((uint64_t)client_jid);
			Task.SetError(-EC_INC_SYNC_STAGE, "register", "inc-sync stage");
			return CACHE_PROCESS_ERROR;
		}
		//����
		else
		{
			log_info("err-sync stage.");

			Task.versionInfo.SetHotBackupID((uint64_t)0);
			Task.SetError(-EC_ERR_SYNC_STAGE, "register", "err-sync stage");
			return CACHE_PROCESS_ERROR;
		}
	}	

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_logout_hb(CTaskRequest &Task)
{
	//TODO����ʱû���뵽logout�ĳ���
	return CACHE_PROCESS_OK;
}

/*
 * ����cache�����е�Node�ڵ�
 */
TCacheResult CCacheProcess::cache_get_key_list(CTaskRequest &Task)
{

	uint32_t lst, lcnt;
	lst  = Task.requestInfo.LimitStart();
	lcnt = Task.requestInfo.LimitCount();

	log_debug("cache_get_key_list start, Limit[%u %u]", lst, lcnt);

	//���������е�Node�ڵ� 
	if(lst > Cache.MaxNodeID())
	{
		Task.SetError(-EC_FULL_SYNC_COMPLETE, "cache_get_key_list", "node id is overflow");
		return CACHE_PROCESS_ERROR;
	}


	Task.PrepareResultNoLimit();

	CRowValue r(Task.TableDefinition());
	CRawData rawdata(&g_stSysMalloc, 1);

	for(unsigned i=lst; i<lst+lcnt; ++i)
	{
		if(i < Cache.MinValidNodeID())
			continue;
		if(i > Cache.MaxNodeID())
			break;

		//���Ҷ�Ӧ��Node�ڵ� 
		CNode node = I_SEARCH(i);
		if(!node)
			continue;
		if(node.NotInLruList())
			continue;
		if(Cache.IsTimeMarker(node))
			continue;

		// ����Key
		CDataChunk *keyptr = M_POINTER(CDataChunk, node.VDHandle());

		//����packedkey
		r[2] = gTableDef[0]->PackedKey(keyptr->Key());

		//����Value
		if(pstDataProcess->GetAllRows(&node, &rawdata))
		{
			rawdata.Destroy();
			continue;
		}

		r[3].Set((char*)(rawdata.GetAddr()), (int)(rawdata.DataSize()));

		Task.AppendRow(&r);

		rawdata.Destroy();
	}

	return  CACHE_PROCESS_OK;
}

/*
 * hot backup��ȡ����key����lru��������û�����������,ֱ��
 * 1. ��ʱ
 * 2. �и���key, ����LRU���
 */
TCacheResult CCacheProcess::cache_get_update_key(CTaskRequest &Task)
{
	log_debug("cache_get_update_key start");

	CJournalID hb_jid    = Task.versionInfo.HotBackupID();
	CJournalID write_jid = hbLog.GetWriterJID();

	//û�и���key����������
	if(hb_jid.GE(write_jid))
	{
		taskPendList.Add2List(&Task);
		return CACHE_PROCESS_PENDING;
	}

	//�Ҳ���hb��������jid������
	if(hbLog.Seek(hb_jid))
	{
		Task.SetError(-EC_BAD_HOTBACKUP_JID, "cache_get_update_key", "jid overflow");

		return CACHE_PROCESS_ERROR;
	}

	Task.PrepareResultNoLimit();

	/* ������ȡhbLog�������룬append��task */
	int count = hbLog.TaskAppendAllRows(Task, 
			Task.requestInfo.LimitCount());

	if(count >= 0)
	{
		statIncSyncStep.push(count);
	}
	else
	{
		Task.SetError(-EC_ERROR_BASE, "CCacheProcess::cache_get_update_key", "decode binlog error");    
		return CACHE_PROCESS_ERROR;     
	}	

	/* ������һ�ε�JournalID */
	Task.versionInfo.SetHotBackupID((uint64_t)hbLog.GetReaderJID());

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_get_raw_data(CTaskRequest &Task)
{
	int iRet;

	const CFieldValue* condition = Task.RequestCondition();
	const CValue* key;

	log_debug("cache_get_raw_data start ");

	CRowValue stRow(Task.TableDefinition()); 	//һ������
	CRawData stNodeData(&g_stSysMalloc, 1);

	Task.PrepareResultNoLimit();

	for(int i=0; i<condition->NumFields(); i++)
	{
		key = condition->FieldValue(i);
		stRow[1].u64 = CHotBackup::HAS_VALUE;       //��ʾ����value�ֶ�
		stRow[2].Set(key->bin.ptr, key->bin.len);

		CNode stNode = Cache.CacheFindAutoChoseHash(key->bin.ptr);
		if(!stNode)
		{//masterû�и�key������
			stRow[1].u64 = CHotBackup::KEY_NOEXIST; 
			stRow[3].Set(0);
			Task.AppendRow(&stRow);
			continue;
		}
		else
		{

			iRet = pstDataProcess->GetAllRows(&stNode, &stNodeData);
			if(iRet != 0)
			{
				log_error("get raw-data failed");
				Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
				return CACHE_PROCESS_ERROR;
			}
			stRow[3].Set((char*)(stNodeData.GetAddr()), (int)(stNodeData.DataSize()));
		}

		Task.AppendRow(&stRow); 	//��ǰ����ӵ�task��
		stNodeData.Destroy();
	}

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_replace_raw_data(CTaskRequest &Task)
{
	log_debug("cache_replace_raw_data start ");

	int iRet;

	const CFieldValue* condition = Task.RequestCondition();
	const CValue* key;

	CRowValue stRow(Task.TableDefinition()); 	//һ������
	CRawData stNodeData(&g_stSysMalloc, 1);
	if(condition->NumFields() < 1){
		log_debug("%s", "replace raw data need key");
		Task.SetErrorDup(-EC_KEY_NEEDED, CACHE_SVC, pstDataProcess->GetErrMsg());
		return CACHE_PROCESS_ERROR;
	}

	key = condition->FieldValue(0);
	stRow[2].Set(key->bin.ptr, key->bin.len);
	Task.UpdateRow(stRow); 	//��ȡ����

	log_debug("value[len: %d]", stRow[3].bin.len);

	//���������Ŀսڵ����
	if(stRow[1].u64&CHotBackup::EMPTY_NODE && m_pstEmptyNodeFilter)
	{
		m_pstEmptyNodeFilter->SET(*(unsigned int*)(key->bin.ptr));
	}

	//key��master������, �����ǿսڵ㣬purge cache.
	if(stRow[1].u64&CHotBackup::KEY_NOEXIST || stRow[1].u64&CHotBackup::EMPTY_NODE)
	{
		log_debug("purge slave data");
		Cache.CachePurge(key->bin.ptr);
		return CACHE_PROCESS_OK;
	}

	// ������raw data
	ALLOC_HANDLE_T hData = g_stSysMalloc.Malloc(stRow[3].bin.len);
	if(hData == INVALID_HANDLE){
		log_error("malloc error: %m");
		Task.SetError(-ENOMEM, CACHE_SVC, "malloc error");
		return CACHE_PROCESS_ERROR;
	}

	memcpy(g_stSysMalloc.Handle2Ptr(hData), stRow[3].bin.ptr, stRow[3].bin.len);

	if((iRet = stNodeData.Attach(hData, 0, tableDef->KeyFormat())) != 0){
		log_error("parse raw-data error: %d, %s", iRet, stNodeData.GetErrMsg());
		Task.SetError(-EC_BAD_RAW_DATA, CACHE_SVC, "bad raw data");
		return CACHE_PROCESS_ERROR;
	}

	// ���packed key�Ƿ�ƥ��
	CValue packed_key = gTableDef[0]->PackedKey(stNodeData.Key());
	if(packed_key.bin.len != key->bin.len || memcmp(packed_key.bin.ptr, key->bin.ptr, key->bin.len))
	{
		log_error("packed key miss match, key size=%d, packed key size=%d",
				key->bin.len, packed_key.bin.len);

		Task.SetError(-EC_BAD_RAW_DATA, CACHE_SVC, "packed key miss match");
		return CACHE_PROCESS_ERROR;
	}

	// ���ҷ���node�ڵ�
	unsigned int uiNodeID;
	CNode stNode = Cache.CacheFindAutoChoseHash(key->bin.ptr);

	if(!stNode){
		for(int i=0; i<2; i++){
			stNode = Cache.CacheAllocate(key->bin.ptr);
			if(!(!stNode))
				break;
			if(Cache.TryPurgeSize(1, stNode) != 0)
				break;
		}
		if(!stNode)
		{
			log_error("alloc cache node error");
			Task.SetError(-EIO, CACHE_SVC, "alloc cache node error");
			return CACHE_PROCESS_ERROR;
		}
		stNode.VDHandle() = INVALID_HANDLE;
	}
	else{
		Cache.RemoveFromLru(stNode);
		Cache.Insert2CleanLru(stNode);	
	}

	uiNodeID = stNode.NodeID();

	// �滻����
	iRet = pstDataProcess->ReplaceData(&stNode, &stNodeData);
	if(iRet != 0){
		if(nodbMode) {
			/* FIXME: no backup db, can't purge data, no recover solution yet */
            log_error("cache replace raw data error: %d, %s", iRet, pstDataProcess->GetErrMsg());
			Task.SetError(-EIO, CACHE_SVC, "ReplaceRawData() error");
			return CACHE_PROCESS_ERROR;	
		} else {
            log_error("cache replace raw data error: %d, %s. purge node: %u", iRet, pstDataProcess->GetErrMsg(), uiNodeID);
			Cache.PurgeNodeEverything(key->bin.ptr, stNode);
			return CACHE_PROCESS_OK;
		}
	}

	Cache.IncTotalRow(pstDataProcess->RowsInc());

	log_debug("cache_replace_raw_data success! ");

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_adjust_lru(CTaskRequest &Task)
{

	const CFieldValue* condition = Task.RequestCondition();
	const CValue* key;

	log_debug("cache_adjust_lru start ");

	CRowValue stRow(Task.TableDefinition()); 	//һ������

	for(int i=0; i<condition->NumFields(); i++){
		key = condition->FieldValue(i);

		CNode stNode;
		int newhash, oldhash;
		if(hashChanging)
		{
			if(targetNewHash)
			{
				oldhash = 0;
				newhash = 1;
			}else
			{
				oldhash = 1;
				newhash = 0;
			}

			stNode = Cache.CacheFind(key->bin.ptr, oldhash);
			if(!stNode)
			{
				stNode = Cache.CacheFind(key->bin.ptr, newhash);
			}else
			{
				Cache.MoveToNewHash(key->bin.ptr, stNode);
			}
		}else
		{
			if(targetNewHash)
			{
				stNode = Cache.CacheFind(key->bin.ptr, 1);
			}else
			{
				stNode = Cache.CacheFind(key->bin.ptr, 0);
			}
		}
		if(!stNode){
			//		            continue;
			Task.SetError(-EC_KEY_NOTEXIST, CACHE_SVC, "key not exist");
			return CACHE_PROCESS_ERROR;	
		}
		Cache.RemoveFromLru(stNode);
		Cache.Insert2CleanLru(stNode);	
	}	

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_verify_hbt(CTaskRequest &Task)
{
	log_debug("cache_verify_hbt start ");

	if(hbFeature == NULL){ // �����ڴ滹û�м����ȱ�����
		NEW(CHBFeature, hbFeature);
		if(hbFeature == NULL){
			log_error("new hot-backup feature error: %m");
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "new hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
		int iRet = hbFeature->Init(0);
		if(iRet == -ENOMEM){
			CNode stNode;
			if(Cache.TryPurgeSize(1, stNode) == 0)
				iRet = hbFeature->Init(0);
		}
		if(iRet != 0){
			log_error("init hot-backup feature error: %d", iRet);
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "init hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
		iRet = Cache.AddFeature(HOT_BACKUP, hbFeature->Handle());
		if(iRet != 0){
			log_error("add hot-backup feature error: %d", iRet);
			Task.SetError(-EC_SERVER_ERROR, "cache_register_hb", "add hot-backup feature fail");
			return CACHE_PROCESS_ERROR;
		}
	}

	int64_t master_timestamp = Task.versionInfo.MasterHBTimestamp();
	if(hbFeature->SlaveUptime() == 0){
		hbFeature->SlaveUptime() = master_timestamp;
	}
	else if(hbFeature->SlaveUptime() != master_timestamp){
		log_error("hot backup timestamp incorrect, master[%lld], this slave[%lld]", (long long)master_timestamp, (long long)(hbFeature->SlaveUptime()));
		Task.SetError(-EC_ERR_SYNC_STAGE, "cache_verify_hbt", "verify hot backup timestamp fail");
		return CACHE_PROCESS_ERROR;
	}

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_get_hbt(CTaskRequest &Task)
{
	log_debug("cache_get_hbt start ");

	if(hbFeature == NULL){ // �����ڴ滹û�м����ȱ�����
		Task.versionInfo.SetMasterHBTimestamp(0);
		Task.versionInfo.SetSlaveHBTimestamp(0);
	}
	else{
		Task.versionInfo.SetMasterHBTimestamp(hbFeature->MasterUptime());
		Task.versionInfo.SetSlaveHBTimestamp(hbFeature->SlaveUptime());
	}

	log_debug("master-up-time: %lld, slave-up-time: %lld", (long long)(Task.versionInfo.MasterHBTimestamp()), (long long)(Task.versionInfo.SlaveHBTimestamp()));

	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_query_serverinfo(CTaskRequest &Task) 
{
	struct CServerInfo s_info;
	memset(&s_info, 0x00, sizeof(s_info));

	s_info.version = 0x1;
	if(hbFeature) {
		CJournalID jid = hbLog.GetWriterJID();
		s_info.binlog_id  = jid.Serial();
		s_info.binlog_off = jid.Offset();
	}

	
	Task.resultInfo.SetServerInfo(&s_info);
	return CACHE_PROCESS_OK;
}

/* finished in one cache process cycle */
TCacheResult CCacheProcess::cache_nodehandlechange(CTaskRequest &Task)
{
	log_debug("cache_nodehandlechange start ");

	const CFieldValue* condition = Task.RequestCondition();
	const CValue* key = condition->FieldValue(0);
	CNode node;
	MEM_HANDLE_T node_handle;
	CRawData node_raw_data(CBinMalloc::Instance(), 0);
	/* no need of private raw data, just for copy */
	char * private_buff = NULL;
	int buff_len;
	MEM_HANDLE_T new_node_handle;

	if(condition->NumFields() < 1){
		log_debug("%s", "nodehandlechange need key");
		Task.SetErrorDup(-EC_KEY_NEEDED, CACHE_SVC, pstDataProcess->GetErrMsg());
		return CACHE_PROCESS_ERROR;
	}

	/* packed key -> node id -> node handle -> node raw data -> private buff*/
	int newhash, oldhash;
	if(hashChanging)
	{
		if(targetNewHash)
		{
			oldhash = 0;
			newhash = 1;
		}else
		{
			oldhash = 1;
			newhash = 0;
		}
		node = Cache.CacheFind(key->bin.ptr, oldhash);
		if(!node)
		{
			node = Cache.CacheFind(key->bin.ptr, newhash);
		}else
		{
			Cache.MoveToNewHash(key->bin.ptr, node);
		}
	}else
	{
		if(targetNewHash)
		{
			node = Cache.CacheFind(key->bin.ptr, 1);
		}else
		{
			node = Cache.CacheFind(key->bin.ptr, 0);
		}
	}

	if(!node)
	{
		log_debug("%s", "key not exist for defragmentation");
		Task.SetError(-ER_KEY_NOT_FOUND, CACHE_SVC, "node not found");
		return CACHE_PROCESS_ERROR;
	}

	node_handle = node.VDHandle();
	if(node_handle == INVALID_HANDLE)
	{
		Task.SetError(-EC_BAD_RAW_DATA, CACHE_SVC, "chunk not exist");
		return CACHE_PROCESS_ERROR;
	}

	node_raw_data.Attach(node_handle, tableDef->KeyFields() - 1, tableDef->KeyFormat());

	if((private_buff = (char *)MALLOC(node_raw_data.DataSize())) == NULL)
	{
		log_error("no mem");
		Task.SetError(-ENOMEM, CACHE_SVC, "malloc error");
		return CACHE_PROCESS_ERROR;
	}

	memcpy(private_buff, node_raw_data.GetAddr(), node_raw_data.DataSize());
	buff_len = node_raw_data.DataSize();
	if(node_raw_data.Destroy())
	{
		log_error("node raw data detroy error");
		Task.SetError(-ENOMEM, CACHE_SVC, "free error");
		FREE_IF(private_buff);
		return CACHE_PROCESS_ERROR;
	}
	log_debug("old node handle: "UINT64FMT_T", raw data size %d", node_handle, buff_len);

	/* new chunk */
	/* new node handle -> new node handle ptr <- node raw data ptr*/
	new_node_handle = CBinMalloc::Instance()->Malloc(buff_len);
	log_debug("new node handle: "UINT64FMT_T, new_node_handle);

	if(new_node_handle == INVALID_HANDLE){
		log_error("malloc error: %m");
		Task.SetError(-ENOMEM, CACHE_SVC, "malloc error");
		FREE_IF(private_buff);
		return CACHE_PROCESS_ERROR;
	}

	memcpy(CBinMalloc::Instance()->Handle2Ptr(new_node_handle), private_buff, buff_len);

	/* free node raw data, set node handle */
	node.VDHandle() = new_node_handle;
	FREE_IF(private_buff);

	log_debug("cache_nodehandlechange success! ");
	return CACHE_PROCESS_OK;
}

TCacheResult CCacheProcess::cache_migrate(CTaskRequest &Task)
{
	if (keyRoute == 0)
	{
		log_error("not support migrate cmd @ bypass mode");
		Task.SetError(-EC_SERVER_ERROR, "cache_migrate", "Not Support @ Bypass Mode");
		return CACHE_PROCESS_ERROR;
	}
	int iRet;

	const CFieldValue * ui = Task.RequestOperation();
	const CValue key = gTableDef[0]->PackedKey(Task.PackedKey());
	if (key.bin.ptr == 0 || key.bin.len <=0)
	{
		Task.SetError(-EC_KEY_NEEDED, "cache_migrate", "need set migrate key");
		return CACHE_PROCESS_ERROR;
	}

	log_debug("cache_cache_migrate start ");

	CRowValue stRow(Task.TableDefinition()); 	//һ������
	CRawData stNodeData(&g_stSysMalloc, 1);


	CNode stNode = Cache.CacheFindAutoChoseHash(key.bin.ptr);

	//�����updateInfo��˵�������TTC����
	int flag = 0;
	if (ui&&ui->FieldValue(0))
	{
		flag = ui->FieldValue(0)->s64;
	}
	if((flag&0xFF) == CMigrate::FROM_SERVER)
	{
		log_debug("this migrate cmd is from TTC");
		CRowValue stRow(Task.TableDefinition()); 	//һ������
		CRawData stNodeData(&g_stSysMalloc, 1);
		stRow[2].Set(key.bin.ptr, key.bin.len);
		Task.UpdateRow(stRow); 	//��ȡ����

		log_debug("value[len: %d]", stRow[3].bin.len);

		//key��master������, �����ǿսڵ㣬purge cache.
		if(stRow[1].u64&CHotBackup::KEY_NOEXIST || stRow[1].u64&CHotBackup::EMPTY_NODE)
		{
			log_debug("purge slave data");
			Cache.CachePurge(key.bin.ptr);
			return CACHE_PROCESS_OK;
		}

		// ������raw data
		ALLOC_HANDLE_T hData = g_stSysMalloc.Malloc(stRow[3].bin.len);
		if(hData == INVALID_HANDLE){
			log_error("malloc error: %m");
			Task.SetError(-ENOMEM, CACHE_SVC, "malloc error");
			return CACHE_PROCESS_ERROR;
		}

		memcpy(g_stSysMalloc.Handle2Ptr(hData), stRow[3].bin.ptr, stRow[3].bin.len);

		if((iRet = stNodeData.Attach(hData, 0, tableDef->KeyFormat())) != 0){
			log_error("parse raw-data error: %d, %s", iRet, stNodeData.GetErrMsg());
			Task.SetError(-EC_BAD_RAW_DATA, CACHE_SVC, "bad raw data");
			return CACHE_PROCESS_ERROR;
		}

		// ���packed key�Ƿ�ƥ��
		CValue packed_key = gTableDef[0]->PackedKey(stNodeData.Key());
		if(packed_key.bin.len != key.bin.len || memcmp(packed_key.bin.ptr,key.bin.ptr, key.bin.len))
		{
			log_error("packed key miss match, key size=%d, packed key size=%d",
					key.bin.len, packed_key.bin.len);

			Task.SetError(-EC_BAD_RAW_DATA, CACHE_SVC, "packed key miss match");
			return CACHE_PROCESS_ERROR;
		}

		// ���ҷ���node�ڵ�
		unsigned int uiNodeID;

		if(!stNode){
			for(int i=0; i<2; i++){
				stNode = Cache.CacheAllocate(key.bin.ptr);
				if(!(!stNode))
					break;
				if(Cache.TryPurgeSize(1, stNode) != 0)
					break;
			}
			if(!stNode)
			{
				log_error("alloc cache node error");
				Task.SetError(-EIO, CACHE_SVC, "alloc cache node error");
				return CACHE_PROCESS_ERROR;
			}
			stNode.VDHandle() = INVALID_HANDLE;
		}
		else{
			Cache.RemoveFromLru(stNode);
			Cache.Insert2CleanLru(stNode);	
		}
		if ((flag>>8)&0xFF) //���Ϊ��ڵ�
		{

			Cache.RemoveFromLru(stNode);
			Cache.Insert2DirtyLru(stNode);
		}

		uiNodeID = stNode.NodeID();

		// �滻����
        iRet = pstDataProcess->ReplaceData(&stNode, &stNodeData);
        if(iRet != 0){
            if(nodbMode) {
                /* FIXME: no backup db, can't purge data, no recover solution yet */
                log_error("cache replace raw data error: %d, %s", iRet, pstDataProcess->GetErrMsg());
                Task.SetError(-EIO, CACHE_SVC, "ReplaceRawData() error");
                return CACHE_PROCESS_ERROR;	
            } else {
                log_error("cache replace raw data error: %d, %s. purge node: %u",
                        iRet, pstDataProcess->GetErrMsg(), uiNodeID);
                Cache.PurgeNodeEverything(key.bin.ptr, stNode);
                return CACHE_PROCESS_OK;
            }
        }

		Cache.IncTotalRow(pstDataProcess->RowsInc());

		Task.PrepareResultNoLimit();

		return CACHE_PROCESS_OK;
	}

	log_debug("this migrate cmd is from api");
	//����ӹ��߹�����������Ҫ�������󷢸�����ttc

	if (!stNode)
	{
		Task.SetError(-EC_KEY_NOTEXIST, "cache_migrate", "this key not found in cache");
		return CACHE_PROCESS_ERROR;
	}
	//��ȡ�ýڵ��raw-data������replace��������helper
	iRet = pstDataProcess->GetAllRows(&stNode, &stNodeData);
	if(iRet != 0)
	{
		log_error("get raw-data failed");
		Task.SetErrorDup(-EIO, CACHE_SVC, pstDataProcess->GetErrMsg());
		return CACHE_PROCESS_ERROR;
	}

	CFieldValue * uitmp = new CFieldValue(4);
	if (uitmp == NULL)
	{
		Task.SetError(-EIO, CACHE_SVC, "migrate:new CFieldValue error");
		return CACHE_PROCESS_ERROR;
	}
	//id0 {"type", DField::Unsigned, 4, CValue::Make(0), 0}
	//type�����һ���ֽ�������ʾ������������ttc����api
	//�����ڶ����ֽڱ�ʾ�ڵ��Ƿ�Ϊ��
	uitmp->AddValue(0,DField::Set,DField::Unsigned,CValue::Make(CMigrate::FROM_SERVER|(stNode.IsDirty()<<8)));

	//id1 {"flag", DField::Unsigned, 1, CValue::Make(0), 0},
	uitmp->AddValue(1,DField::Set,DField::Unsigned,CValue::Make(CHotBackup::HAS_VALUE));
	//id2 {"key", DField::Binary, 255, CValue::Make(0), 0},

	//id3 {"value", DField::Binary, MAXPACKETSIZE, CValue::Make(0), 0},

	FREE_IF(Task.migratebuf);
	Task.migratebuf = (char *)calloc(1,stNodeData.DataSize());
	if (Task.migratebuf == NULL)
	{
		log_error("create buffer failed");
		Task.SetError(-EIO, CACHE_SVC, "migrate:get raw data,create buffer failed");
		return CACHE_PROCESS_ERROR;
	}
	memcpy(Task.migratebuf,(char*)(stNodeData.GetAddr()),(int)(stNodeData.DataSize()));
	uitmp->AddValue(3,DField::Set,DField::Binary,
			CValue::Make(Task.migratebuf,stNodeData.DataSize()));
	Task.SetRequestOperation(uitmp);

	return CACHE_PROCESS_REMOTE;
}
