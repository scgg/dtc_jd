/*
 * =====================================================================================
 *
 *       Filename:  check_hash_node_info.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/21/2010 12:38:19 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */


#include <string.h>
#include "check_hash_node_info.h"
#include "check_cache_holder.h"
#include "check_code.h"
#include "check_result.h"


//for color contorl... 装13用
#define COLOR 0 
#if COLOR
#define RED "[1m[31m"
#define DARKRED "[31m"
#define GREEN "[1m[32m"
#define DARKGREEN "[32m"
#define DARKBLUE "[34m"
#define DARKYELLOW "[33m"
#define MAGENTA "[1m[35m"
#define DARKMAGENTA "[1m[36m"
#define CYAN "[36m"
#define RESET "[m"
#else
#define RED ""
#define DARKRED ""
#define GREEN ""
#define DARKGREEN ""
#define DARKBLUE ""
#define DARKYELLOW ""
#define MAGENTA ""
#define DARKMAGENTA ""
#define CYAN ""
#define RESET ""
#endif
TTC_BEGIN_NAMESPACE

CHashPrinter::CHashPrinter (void) : _hash(NULL)
{

}

CHashPrinter::~CHashPrinter (void)
{

}

int CHashPrinter::open (int argc, char** argv)
{
    /* set check name */
    strncpy (_name, HASH_NODE_INFO_PRINTER_NAME, strlen(HASH_NODE_INFO_PRINTER_NAME));

    /* get hash */
    _hash = CCacheHolder::Instance()->get_hash();
    if (NULL == _hash)
    {
        log_error ("%s", "get hash instance failed");
        return CHECK_ERROR_HASH;
    }

    return CHECK_SUCC;
}

int CHashPrinter::close (void)
{
    /* clean name */
    memset (_name, 0x00, sizeof(_name));

    /* clean hash instance */
    _hash = NULL;

    return CHECK_SUCC;
}

uint32_t CHashPrinter::NG_free(NODE_GROUP_T * NG)
{
   uint32_t free = NG->ng_dele.count;
   if (NG->ng_free < NODE_GROUP_INCLUDE_NODES)
       free += (NODE_GROUP_INCLUDE_NODES - NG->ng_free);
    return free;
}


int CHashPrinter::check(void* parameter)
{
    uint32_t        hash_size   = _hash->HashSize();
    uint32_t        free_buckets   = _hash->FreeBucket();

    printf(GREEN"\nHASH PRINT START!!!!!!!!!!"RESET"\n");
    printf(RED"hash globle:"RESET"\n");
    printf("  hashsize["DARKYELLOW"%u"RESET"]\t"
            "free_buckets["DARKYELLOW"%u"RESET"]\t"
            "used_buckets["DARKYELLOW"%u"RESET"]\n",
            hash_size,free_buckets,hash_size-free_buckets);
    MEM_HANDLE_T    mem_handle  = INVALID_HANDLE;
    CNode 	    node;
    int count = 0;

    printf(RED"hash detail:\n"RESET"\t\t[%sclean"RESET" %s*dirty"RESET"]\n",DARKMAGENTA,DARKYELLOW);

    printf(MAGENTA"  [bucket]\t[node_id]\t[handle]\tNGid[used/total]\tnodes in bucket"RESET"\n");

    //遍历
    for (uint32_t i = 0; i < hash_size; ++i)
    {
        node = I_SEARCH(_hash->Hash2Node(i));
        count = 0;

        while (!(!node))
        {
            mem_handle = node.VDHandle();
            if (INVALID_HANDLE == mem_handle)
            {
                /* save handle */
                CCheckResult::Instance()->save_hash_handle(node.NodeID(), mem_handle);
                log_error ("hash's bucket[%u] node_id[%u], handle["UINT64FMT"] is invalid", i, node.NodeID(), mem_handle);
                node = node.NextNode();
                continue;
            }

            if (CCacheHolder::Instance()->get_nginfo()->CleanNodeHead() == node)
            {
                log_error ("hash's bucket[%u] node_id[%u], handle["UINT64FMT"] is invalid", i, node.NodeID(), mem_handle);
                node = node.NextNode ();
                continue;
            }

            if (CBinMalloc::Instance()->HandleIsValid(mem_handle) != 0)
            {
                /* save handle */
                CCheckResult::Instance()->save_hash_handle(node.NodeID(), mem_handle);
                log_error ("hash's bucket[%u] node_id[%u], handle["UINT64FMT"] is invalid", i, node.NodeID(), mem_handle);
            }

            if (!count++)//the first line,print bucket index
            {
                if(!!node.NextNode())//有多行数据
                {
                    printf("  ["DARKYELLOW"%u"RESET"]" //buckets index
                        "\t\t[%s%u"RESET"]"//color and node id
                        "\t\t["DARKYELLOW""UINT64FMT""RESET"]"//mem handle
                        "\t%u[%u/256]\n",//ng id[used/total]
                        i, node.IsDirty()?DARKYELLOW"*":DARKMAGENTA,node.NodeID(),
                        mem_handle,node.Owner()->ng_nid,256-NG_free(node.Owner()));
                }
                else
                {
                    printf("  ["DARKYELLOW"%u"RESET"]" //buckets index
                        "\t\t[%s%u"RESET"]"//color and node id
                        "\t\t["DARKYELLOW""UINT64FMT""RESET"]"//mem handle
                        "\t%u[%u/256]"//ng info[used/total]
                        RED"\t\t%d nodes"RESET"\n",//total nodes in bucket
                        i, node.IsDirty()?DARKYELLOW"*":DARKMAGENTA,node.NodeID(),
                        mem_handle,node.Owner()->ng_nid,256-NG_free(node.Owner()),count);
                }

            }
            else//next lines
            {
                if(!!node.NextNode())
                {
                    printf("  \t\t[%s%u"RESET"]"//color and node id
                            "\t\t["DARKYELLOW""UINT64FMT""RESET"]"//mem handle
                            "\t%u[%u/256]\n",//ng info[used/total]
                             node.IsDirty()?DARKYELLOW"*":DARKMAGENTA,node.NodeID(),
                            mem_handle,node.Owner()->ng_nid,256-NG_free(node.Owner()));
                }
                else//如果是最后一行，打印bucket里的节点总数
                {
                    printf("  \t\t[%s%u"RESET"]"//color and node id
                            "\t\t["DARKYELLOW""UINT64FMT""RESET"]"//mem handle
                            "\t%u[%u/256]"//ng info[used/total]
                            RED"\t\t%d nodes"RESET"\n",//total nodes in bucket
                            node.IsDirty()?DARKYELLOW"*":DARKMAGENTA,node.NodeID(),
                            mem_handle,node.Owner()->ng_nid,256-NG_free(node.Owner()),count);
                }
            }
            node = node.NextNode();
        }
    }

    printf(GREEN"HASH PRINT FINISH!!!!!!!!!!"RESET"\n\n");
    log_error ("%s", "hash bucket is finished.");

    return CHECK_SUCC;
}

TTC_END_NAMESPACE
