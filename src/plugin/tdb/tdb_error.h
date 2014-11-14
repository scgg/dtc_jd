#ifndef __TDB_ERROR_H
#define __TDB_ERROR_H

#define	 E_TDB_BASE								21000	// 错误base
#define	 E_TDB_PHYSIC_PAGE_NO					21001	// 错误的物理页号
#define	 E_TDB_ALLOC_PHYSIC_PAGE		    	21002	// 无法分配物理页
#define	 E_TDB_NO_BUCKET						21003	// 无对应桶
#define	 E_TDB_BUCKET_BUFSIZE					21004	// 桶分配的bufsize太小
#define	 E_TDB_ALLOC_OVERFLOW_BLOCK	            21005	// 无法分配溢出块
#define	 E_TDB_NO_RECORD						21006	// 无对应记录
#define	 E_TDB_RECORD_BUFSIZE					21007	// 记录分配的bufsize太小
#define	 E_TDB_NO_TABLE							21008	// 无对应的table
#define	 E_TDB_EXSIT_TABLE						21009	// table已经存在
#define	 E_TDB_ENOUGH_PAGE						21010	// 无足够的物理页
#define	 E_TDB_NO_EXSIT_DB						21011	// db不存在
#define	 E_TDB_EXSIT_DB							21012	// db已存在
#define  E_TDB_DUMP_DB_CONF                     21013   // dump db conf 错误
#define  E_TDB_DBID_ARG		                    21014   // dbid 不合法
#define  E_TDB_TABLEID_ARG		                21015   // tableid 不合法
#define  E_TDB_DB_NO_EMPTY		        21016   // db不为空
#define  E_TDB_BLOCK_LEN_ARG                    21017   // 平均记录大小不合法
#define  E_TDB_DB_BUSY                          21018   // 过载
#define  E_TDB_WCACHE_EXSIT                     21019   // 写cache不存在
#define  E_TDB_WCACHE_DEL                       21020   // 写cache已经删除 
#define  E_TDB_DECODE                           21030   // 解码错误
#define  E_TDB_SYNCING                          21040   // 同步的时候，发现该记录同步中
#define  E_TDB_GET_CACAHE_SIZE                  21050   // 获取写cache，发现超过16M
#define  E_TDB_RECORD_SIZE_ZERO                 21060   


#define E_HAND_BASE                            20000
#define E_HAND_MASTER                          20001
#define E_HAND_CELL_1                          20002
#define E_HAND_CELL_2                          20003
#define E_HAND_RO                              20004
#define E_HAND_NO_DB                           20005
#define E_HAND_NO_TABLE                        20006
#define E_HAND_NO_CELL_TABLET                  20007
#define E_HAND_NO_RANGE                        20008

#endif
