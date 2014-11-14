/*
 * =====================================================================================
 *
 *       Filename:  registor.cc
 *
 *    Description:  
 *
 *        Version:  0.2
 *        Created:  05/23/2010 02:03:46 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */

#include "comm.h" 
#include "registor.h"
int CRegistor::Regist() {
    TTC::SvrAdminRequest rq(_master);
    rq.SetAdminCode(TTC::RegisterHB);

    /* 发送自己的JournalID */
    CJournalID self = _controller.JournalID();
    rq.SetHotBackupID((uint64_t) self);

    TTC::Result rs;
    rq.Execute(rs);

    switch (rs.ResultCode()) {
    case -TTC::EC_INC_SYNC_STAGE:
        {
            log_notice("server report: \"INC-SYNC\"");

            /* 
             * 增量同步要求master、slave的共享内存创建时间必须相等
             * 而且不能为0
             *
             */
            _master_ctime = QueryMemoryCreateTime(_master, 1);
            _slave_ctime  = QueryMemoryCreateTime(_slave,  0);

            if (_master_ctime <= 0 || _slave_ctime <= 0 ||
                    _master_ctime != _slave_ctime) {
                log_debug("master mem time: "UINT64FMT", slave mem time: "UINT64FMT,
                        _master_ctime, _slave_ctime);
                log_error
                    ("share memory create time changed");
                return -TTC::EC_ERR_SYNC_STAGE;
            }

            return -TTC::EC_INC_SYNC_STAGE;
        }

    case -TTC::EC_FULL_SYNC_STAGE:
        {
            log_notice("server report: \"FULL-SYNC\"");

            /*
             * 全量同步要求master的共享内存创建时间不为0，
             * slave的共享内存创建时间必须为0
             *
             * bitmap无法判断创建时间，所以bitmap全部返回1，而ttc的时间
             * 取的是time的结果，所以当_slave_ctime > 10的时候，就说明
             * slave的数据不干净。
             */
            _master_ctime = QueryMemoryCreateTime(_master, 1);
            _slave_ctime  = QueryMemoryCreateTime(_slave, 0);
            if (_master_ctime <= 0 || _slave_ctime > 10) {
                log_debug("master mem time: "UINT64FMT", slave mem time: "UINT64FMT,
                        _master_ctime, _slave_ctime);
                log_error
                    ("slave have dirty data. must purge first");
                return -TTC::EC_ERR_SYNC_STAGE;
            }

            /* 再次验证并且更新slave的创建时间 */
            if (VerifyMemoryCreateTime(_master_ctime, _master_ctime)) {
                return -TTC::EC_ERR_SYNC_STAGE;
            }
            _slave_ctime = _master_ctime;

            /* 更新控制文件中的journalID */
            _controller.JournalID() = rs.HotBackupID();

            log_info
                ("registed to master, master[serial=%u, offset=%u]",
                 _controller.JournalID().serial,
                 _controller.JournalID().offset);

            return -TTC::EC_FULL_SYNC_STAGE;
        }

    default:
        {
            log_notice("server report: \"ERR-SYNC\"");
            return -TTC::EC_ERR_SYNC_STAGE;
        }
    }
}
