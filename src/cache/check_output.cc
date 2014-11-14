/*
 * =====================================================================================
 *
 *       Filename:  check_output.cc
 *
 *    Description:  check output impl 
 *
 *        Version:  1.0
 *        Created:  03/19/2009 02:34:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  frankyang (huanhuange), frankyang@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "check_output.h"
#include "singleton.h"
#include "check_result.h"
#include "check_code.h"
#include "check_lru_node_info.h"
#include "check_result_info.h"

TTC_BEGIN_NAMESPACE

CCheckOutput::CCheckOutput (void)
{

}

CCheckOutput::~CCheckOutput (void)
{

}

CCheckOutput* CCheckOutput::Instance (void)
{
    return CSingleton<CCheckOutput>::Instance ();
}

void CCheckOutput::Destroy (void)
{
    return CSingleton<CCheckOutput>::Destroy ();
}

int CCheckOutput::open (void)
{
    return 0;
}

int CCheckOutput::close (void)
{
    return 0;
}

int CCheckOutput::output (void)
{
//    CCheckResult::NODE_CONTAINER_T& hash_nodes  = CCheckResult::Instance()->get_hash_nodes (); 
//    CCheckResult::NODE_CONTAINER_T& lru_nodes   = CCheckResult::Instance()->get_lru_nodes (); 
    CCheckResult::LRU_NODE_INFO_T & dirty_lru = CCheckResult::Instance()->get_dirty_lru_node_info();
    CCheckResult::LRU_NODE_INFO_T & dirty_lru_reverse =  CCheckResult::Instance()->get_dirty_lru_node_info_reverse();
    CCheckResult::LRU_NODE_INFO_T & clean_lru = CCheckResult::Instance()->get_clean_lru_node_info();
    CCheckResult::LRU_NODE_INFO_T & clean_lru_reverse =  CCheckResult::Instance()->get_clean_lru_node_info_reverse();
    CCheckResult::LRU_NODE_INFO_T & empty_lru = CCheckResult::Instance()->get_empty_lru_node_info();
    CCheckResult::LRU_NODE_INFO_T & empty_lru_reverse =  CCheckResult::Instance()->get_empty_lru_node_info_reverse();

/*    // hash node info 
    fprintf (stderr, "Hash Node Error Information: (total: "UINT64FMT")\n", (uint64_t)hash_nodes.size());
    fprintf (stderr, "==================================================\n");
    fprintf (stderr, "Node ID\t\t\t\tMemory Handle\n");

    for (uint32_t i = 0; i < hash_nodes.size(); ++i)
    {
        fprintf (stderr, "%u\t\t\t"UINT64FMT"\n", hash_nodes[i]._node_id, hash_nodes[i]._mem_handle);
    }

    fprintf (stderr, "\n\n");

    // lru node info 
    fprintf (stderr, "LRU Node Error Information: (total: "UINT64FMT")\n", (uint64_t)lru_nodes.size());
    fprintf (stderr, "==================================================\n");
    fprintf (stderr, "Node ID\t\t\t\tMemory Handle\n");

    for (uint32_t i = 0; i < lru_nodes.size(); ++i)
    {
        fprintf (stderr, "%u\t\t\t"UINT64FMT"\n", lru_nodes[i]._node_id, lru_nodes[i]._mem_handle);
    }

    fprintf (stderr, "\n\n");

    // hash & lru node diff 
    fprintf (stderr, "Next & Prev LRU Node Information:\n");
    fprintf (stderr, "==================================================\n");
    fprintf (stderr, "Next LRU Node Count\t\tPrev LRU Node Count\t\t\tDiff\n");
    fprintf (stderr, UINT64FMT"\t\t\t\t"UINT64FMT"\t\t\t\t\t"INT64FMT,
             CCheckResult::Instance()->get_next_count(),
             CCheckResult::Instance()->get_prev_count(),
             CCheckResult::Instance()->node_count_diff());
    fprintf (stderr, "\n\n");
*/


    //print dirty lru list 
    const char * dirty_flag[2] = {"clean", "dirty"};
    std::map<int, const char *> row_flag_map;
    row_flag_map[0] = "cleared";
    row_flag_map[48] = "select";
    row_flag_map[50] = "update";
    row_flag_map[54] = "insert";
    row_flag_map[2]  = "dirty";
    row_flag_map[49] = "insert_old";
    row_flag_map[51] = "delete_na";
    row_flag_map[52] = "flush";
    row_flag_map[53] = "reserve1";
    row_flag_map[55] = "reserve2"; 

    CCheckResult::LRU_NODE_INFO_T * lru_list[3] = {&dirty_lru, &clean_lru, &empty_lru};
    CCheckResult::LRU_NODE_INFO_T * lru_list_reverse[3] = {&dirty_lru_reverse, &clean_lru_reverse, &empty_lru_reverse};
    
    printf("====================================format\n");
    printf("marker format:\n[node id]\t[marker time]\n\n");
    printf("normal node format:\n[node id]\t[key]\t[dirty flag]\t[i]\t[dirty flag]\n");
    printf("\t\t\t\t\t[i]\t[dirty flag]\n\n");
    int it = 0;
    for(it = 0 ; it < 3; it ++)
    {
        switch(it)
        {
            case 0:
                printf("======================================dirty\n");
                break;
            case 1:
                printf("======================================clean\n");
                break;
            case 2:
                printf("======================================empty\n");
                break;
        }
        
        CCheckResult::LRU_NODE_INFO_T & this_lru = *lru_list[it];

        for(unsigned int i = 0 ; i < this_lru.size(); i ++)
        {
            int node_dirty = this_lru[i]._dirty_flag;

            if(this_lru[i]._time != 0)
            {
                fprintf(stderr, "[%d]\t["UINT64FMT"]\n", this_lru[i]._node_id,this_lru[i]._time);        
                continue;
            }else
            {
                fprintf(stderr, "[%d]\t[%lld]\t[%s]\t", this_lru[i]._node_id, this_lru[i]._key, dirty_flag[node_dirty]);  
            }

            int row_flag = 0;
            if(this_lru[i]._row_flags.size() == 0)
            {
                fprintf(stderr, "\n");
                continue;
            }else
            {
                row_flag = this_lru[i]._row_flags[0];
                fprintf(stderr, "\t[%d]\t[%s]\n", 0, row_flag_map[row_flag]);
            }

            for(unsigned int j = 1; j < this_lru[i]._row_flags.size(); j ++)
            {
                row_flag = this_lru[i]._row_flags[j];
                fprintf(stderr, "\t\t\t\t[%d]\t[%s]\n", j, row_flag_map[row_flag]);
            }
        }

        printf("\n\n");
    }

    for(it = 0 ; it < 3; it ++)
    {
        CCheckResult::LRU_NODE_INFO_T & this_lru = *lru_list_reverse[it];
        switch(it)
        {
            case 0:
                printf("======================================dirty reverse\n");
                break;
            case 1:
                printf("======================================clean reverse\n");
                break;
            case 2:
                printf("======================================empty reverse\n");
                break;
        }
        for(unsigned int i = 0 ; i < this_lru.size(); i ++)
        {
            int node_dirty = this_lru[i]._dirty_flag;

            if(this_lru[i]._time != 0)
            {
                fprintf(stderr, "[%d]\t["UINT64FMT"]\n", this_lru[i]._node_id, this_lru[i]._time);        
                continue;
            }else
            {
                fprintf(stderr, "[%d]\t[%lld]\t[%s]\t", this_lru[i]._node_id, this_lru[i]._key, dirty_flag[node_dirty]);  
            }

            int row_flag = 0;
            if(this_lru[i]._row_flags.size() == 0)
            {
                fprintf(stderr, "\n");
                continue;
            }else
            {
                row_flag = this_lru[i]._row_flags[0];
                fprintf(stderr, "\t[%d]\t[%s]\n", 0, row_flag_map[row_flag]);
            }

            for(unsigned int j = 1; j < this_lru[i]._row_flags.size(); j ++)
            {
                row_flag = this_lru[i]._row_flags[j];
                fprintf(stderr, "\t\t\t\t[%d]\t[%s]\n", j, row_flag_map[row_flag]);
            }
        }
        printf("\n\n");
    }
    return CHECK_SUCC;
}

TTC_END_NAMESPACE
