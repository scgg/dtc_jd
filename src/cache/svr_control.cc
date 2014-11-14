#include <svr_control.h>
#include <log.h>
#include "protocol.h"

CServerControl * CServerControl::serverControl = NULL;

CServerControl::CServerControl(CPollThread *o) : CTaskDispatcher<CTaskRequest>(o), m_output(o)
{
    atomic8_set(&m_readOnly, 0);
    m_statReadonly = statmgr.GetItemU32(SERVER_READONLY);
    m_statReadonly.set( (0 == atomic8_read(&m_readOnly))?0:1);
}

CServerControl::~CServerControl(void)
{
}

CServerControl * CServerControl::GetInstance(CPollThread *o)
{
    if(NULL == serverControl)
    {
        NEW(CServerControl(o), serverControl);
    }
    return serverControl;
}

CServerControl * CServerControl::GetInstance()
{        
    return serverControl;
}

bool CServerControl::IsReadOnly()
{
    return 0 != atomic8_read(&m_readOnly);
}
void CServerControl::QueryMemInfo(CTaskRequest *cur)
{
	struct CServerInfo s_info;
	memset(&s_info, 0x00, sizeof(s_info));

	s_info.version = 0x1;
	s_info.datasize = statmgr.Get10SItemValue(TTC_DATA_SIZE);
	s_info.memsize = statmgr.Get10SItemValue(TTC_CACHE_SIZE);
	log_debug("Memory info is: memsize is %lu , datasize is %lu",s_info.memsize,  s_info.datasize);
	cur->resultInfo.SetServerInfo(&s_info);
	
}
void CServerControl::dealServerAdmin(CTaskRequest *cur)
{
    switch(cur->requestInfo.AdminCode())
    {
    case DRequest::ServerAdminCmd::SET_READONLY:
        {
        atomic8_set(&m_readOnly, 1);
        m_statReadonly.set(1);
        log_info("set server status to readonly.");
        break;
        }
    case DRequest::ServerAdminCmd::SET_READWRITE:
        {
        atomic8_set(&m_readOnly, 0);
        m_statReadonly.set(0);
        log_info("set server status to read/write.");
        break;
        }
         case DRequest::ServerAdminCmd::QUERY_MEM_INFO:
        {
        		log_debug("query meminfo.");
       		QueryMemInfo(cur);
        		break;
        }
        
    default:
        {
		log_debug("unknow cmd: %d", cur->requestInfo.AdminCode());
        cur->SetError(-EC_REQUEST_ABORTED, "RequestControl", "Unknown svrAdmin command.");
        break;
        }
    }

    cur->ReplyNotify();
}

void CServerControl::TaskNotify(CTaskRequest *cur)
{
   log_debug("CServerControl::TaskNotify Cmd is %d, AdminCmd is %u", cur->RequestCode(), cur->requestInfo.AdminCode());
    //处理ServerAdmin命令
    if(DRequest::SvrAdmin == cur->RequestCode())
    {
		switch(cur->requestInfo.AdminCode())
		{
			case DRequest::ServerAdminCmd::SET_READONLY:
			case DRequest::ServerAdminCmd::SET_READWRITE:
			case DRequest::ServerAdminCmd::QUERY_MEM_INFO:
				dealServerAdmin(cur);
				return;
			
				
		//allow all AdminCode pass	
			default:
				{
					 log_debug("CServerControl::TaskNotify admincmd,  tasknotify next process ");
					m_output.TaskNotify(cur);
					return;
				}
				
		}
    }

    //当server为readonly，对非查询请求直接返回错误
    if(0 != atomic8_read(&m_readOnly))
    {
        if(DRequest::Get != cur->RequestCode())
        {
	    log_info("server is readonly, reject write operation");
            cur->SetError(-EC_SERVER_READONLY, "RequestControl", "Server is readonly.");
            cur->ReplyNotify();
            return;         
        }
    }
    log_debug("CServerControl::TaskNotify tasknotify next process ");
    m_output.TaskNotify(cur);
}

