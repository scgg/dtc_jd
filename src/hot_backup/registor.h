/**
 * @brief 向 master 发起注册请求
 *
 * @file registor.h
 * @author jackda
 * @time 17/11/08 10:42:41
 * @version 0.1
 *
 * CopyRight (C) jackda(答治茜) @2007~2010@
 *
 **/

#ifndef __HB_REGISTOR_H
#define __HB_REGISTOR_H

#include "async_file.h"
#include "ttcapi.h"
#include "log.h"

class CRegistor {
public:
	CRegistor(): _master(0), _slave(0), _master_ctime(-1), _slave_ctime(-1) {

	} ~CRegistor() {

	}

	int Init(TTC::Server * m, TTC::Server * s) {
		_master = m;
		_slave = s;
		return _controller.Init();
	}
    int Regist();

	/*
	 * 定期检查双方的共享内存，确保二者始终没有变化过。
	 *
	 * 如果slave上记录的内存创建时间和master共享内存
	 * 创建时间相同，证明二者的内存自创建以来，没有
	 * 被删除过，返回0， 否则返回1
	 */
	int CheckMemoryCreateTime() {
		int64_t v0, v1;

		if (_master_ctime <= 0 || _slave_ctime <= 0) {
			log_info("please invoke \"Regist\" function first");
			return 0;
		}

		v0 = QueryMemoryCreateTime(_master, 1);
		if (v0 > 0) {
			if (v0 != _master_ctime) {
				log_error("master memory changed");
				return -1;
			}
		}

		v1 = QueryMemoryCreateTime(_slave, 0);
		if (v1 > 0) {
			if (v1 != _slave_ctime) {
				log_error("slave memory changed");
				return -1;
			}
		}

		return 0;
	}
#ifdef UINT64FMT
# undef UINT64FMT
#endif
#if __WORDSIZE == 64
# define UINT64FMT "%lu"
#else
# define UINT64FMT "%llu"
#endif

	/* 
	 * 设置hbp状态为 "全量同步未完成" 
	 * 当hbp出现任何不可恢复的错误时，应该invoke这个接口
	 *
	 */
	void SetHBPStatusDirty() {
		_controller.SetDirty();
	}

private:

	inline int VerifyMemoryCreateTime(long long m, long long s) {
		TTC::SvrAdminRequest rq(_slave);
		rq.SetAdminCode(TTC::VerifyHBT);

		rq.SetMasterHBTimestamp(m);
		rq.SetSlaveHBTimestamp(s);

		TTC::Result rs;
		rq.Execute(rs);

		if (rs.ResultCode() == -TTC::EC_ERR_SYNC_STAGE) {
			return -1;
		}

		return 0;
	}

	inline int64_t QueryMemoryCreateTime(TTC::Server * svr, int master) {
		TTC::SvrAdminRequest rq(svr);
		rq.SetAdminCode(TTC::GetHBTime);

		TTC::Result rs;
		rq.Execute(rs);

		if (rs.ResultCode() != 0)
			return -1;

		if(master)
			return rs.MasterHBTimestamp();
		else
			return rs.SlaveHBTimestamp();
	}

private:
	TTC::Server * _master;
	TTC::Server * _slave;
	CAsyncFileController _controller;
	/* master memory create time */
	int64_t _master_ctime;
	int64_t _slave_ctime;
};

#endif
