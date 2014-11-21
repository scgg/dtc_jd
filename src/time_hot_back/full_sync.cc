#include <stdio.h>
#include "full_sync.h"
#include "daemon.h"
#include "value.h"

int CFullSync::Init()
{
	if (_controller.Init()) {
		snprintf(_errmsg, sizeof(_errmsg), "controller init failed, %s",
			 _controller.ErrorMessage());

		return -1;
	}

	return 0;
}

/*
 * 全量同步写入slave
 */
int CFullSync::Run()
{
	//设置全量初始化状态
	_controller.SetDirty();

	while (!DaemonBase::_stop) {
		TTC::SvrAdminRequest request_m(_master);
		request_m.SetAdminCode(TTC::GetKeyList);

		/* client需要Need 字段, 否则server的AppendRows会过滤掉 */
		request_m.Need("key");
		request_m.Need("value");

		/*
		 * 对ttc来说，start是起始Node ID
		 * 对bitmap来说，start是 key
		 */
		request_m.Limit(_start, _limit);

		TTC::Result result_m;

		int ret = request_m.Execute(result_m);

		//全量key同步完成
		if (TTC::EC_FULL_SYNC_COMPLETE == -ret) {
			//设置全量初始化状态为完成
			_controller.ClrDirty();
			return 0;
		}
		//出错， 要不要重试？？
		else if (0 != ret) {
			snprintf(_errmsg, sizeof(_errmsg),
				 "fetch key-list from master failed, limit[%d %d], ret=%d, err=%s",
				 _start, _limit, ret, result_m.ErrorMessage());
			goto ERR;
		}
		//同步到备机
		for (int i = 0; i < result_m.NumRows(); ++i) {
			ret = result_m.FetchRow();
			if (ret < 0) {
				snprintf(_errmsg, sizeof(_errmsg),
					 "fetch key-list from master failed, limit[%d %d], ret=%d, err=%s",
					 _start, _limit, ret,
					 result_m.ErrorMessage());

				/*
				 * ttc可以运行失败，但bitmapsvr是不允许失败的，如何处理？
				 *
				 */
				goto ERR;
			}

			TTC::SvrAdminRequest request_s(_slave);
			request_s.SetAdminCode(TTC::ReplaceRawData);

			CValue v;

			v.bin.ptr =
			    (char *)result_m.BinaryValue("key", &(v.bin.len));
			request_s.EQ("key", v.bin.ptr, v.bin.len);

			v.bin.ptr =
			    (char *)result_m.BinaryValue("value", &(v.bin.len));
			request_s.Set("value", v.bin.ptr, v.bin.len);

			log_debug("value: " UINT64FMT ", bytes:%d",
				  *(uint64_t *) v.bin.ptr, v.bin.len);

			TTC::Result result_s;
			if (0 != request_s.Execute(result_s)
			    || 0 != result_s.ResultCode()) {
				snprintf(_errmsg, sizeof(_errmsg),
					 "sync key-list to slave failed, ret=%d, err=%s",
					 ret, result_s.ErrorMessage());
				/*
				 * ttc可以运行失败，但bitmapsvr是不允许失败的，如何处理？
				 *
				 */
				goto ERR;
			}
		}

		_start += _limit;
	}

ERR:
	return -1;
}
