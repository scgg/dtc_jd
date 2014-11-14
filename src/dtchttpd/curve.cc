#include "curve.h"
#include "log.h"
#include <sstream>
#include "helper.h"
#include "LockFreeQueue.h"

extern LockFreeQueue<MSG> g_Queue;

int Curve::ProcessData(const MSG& data)
{
	time_iterator it = time_map.find(data.bid);
	if (time_map.end() != it) //再次插入
	{
		do_iterator data_it = do_map.find(data.bid);
		log_info("bid: %d last: %u now: %ld", data.bid, it->second, data.datetime);
		if (Expire(it->second, data.datetime)) //超过10s
		{
			log_info("bid: %d more than 10s", data.bid);
			//write to queue
			log_info("before enqueue, queue size is %d", g_Queue.Size());
			bool ret = g_Queue.EnQueue(data_it->second);
			log_info("after enqueue, queue size is %d", g_Queue.Size());
			if (true == ret)
			{
				log_info("insert into queue success.");
				//更新time_map
				it->second = data.datetime;
				log_info("bid: %d datetime: %u", data.bid, it->second);
				//更新 do_map
				(data_it->second).data1 = data.data1;
				(data_it->second).data2 = data.data2;
				(data_it->second).extra = data.extra;
				(data_it->second).datetime = data.datetime;
				log_info("bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", (data_it->second).bid, 
						 (data_it->second).curve, (data_it->second).etype, (data_it->second).data1, (data_it->second).data2, 
						 (data_it->second).extra, (data_it->second).datetime);
				return 0;
			}
			else
			{
				log_error("insert into queue failed");
				return -1;
			}
		}
		else //未超10s合并
		{
			log_info("bid: %d less than 10s", data.bid);
			(data_it->second).data1 += data.data1;
			(data_it->second).data2 += data.data2;
			(data_it->second).extra += data.extra;
			log_info("bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", (data_it->second).bid, (data_it->second).curve, 
					 (data_it->second).etype, (data_it->second).data1, (data_it->second).data2, (data_it->second).extra, (data_it->second).datetime);
			return 0;
		}
	}
	else //首次插入
	{
		time_map[data.bid] = data.datetime;
		do_map[data.bid] = data;
		log_info("bid: %d datetime: %ld", data.bid, data.datetime);
		log_info("bid: %d curve: %d etype: %d data1: %lld data2: %lld extra: %lld datetime: %ld", 
				 data.bid, data.curve, data.etype, data.data1, data.data2, data.extra, data.datetime);
		return 0;
	}
}

bool Curve::Expire(long last, long now)
{
	if ((now-last) >= 10)
		return true;
	else
		return false;
}
