#ifndef _TDB_HANDLE_H_
#define _TDB_HANDLE_H_

#include <string>
#include <vector>
#include <map>
#include <time.h>

#include "tdb_handle_base.h"
namespace tdb { namespace api {

class CTdbApi;

/*
 * hand_ip : 接入机器的ip
 * route_ip : 路由server的ip
 * cell_ip : 实际存储数据的ip 
 */
class CTdbHandle
{
private:
//	CTdbApi* sc;

	typedef struct
	{
		time_t tValidTime;
		CTdbApi* sc;
	}TTdbApiStu;
	
	std::vector<TTdbApiStu> _vec_tdbapi;
	int m_iFreezeSecs;
public:
	CTdbHandle(const std::string& hand_ip);
	
	CTdbHandle();
	virtual ~CTdbHandle();
	
	//设置不可用ip的冻结时间
	void SetFreezeLinkSecs(int iFreezeSecs);
	//增加新地址	
	void AddAddr(const std::string& hand_ip);	
	
	void SetTime(unsigned send_time, unsigned recv_time/*ms*/);
	void GetTime(unsigned& send_time, unsigned& recv_time/*ms*/);
	void SetAddr(const std::string& hand_ip);
	std::string GetAddr();

	void SetRouteAddr(const std::string& rout_ip);
	std::string GetRouteAddr();
	
	/*************************************************
	 * Function:       Redirect
	 * Description:    根据key，获取对应存储机器的ip
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 key, 记录的键
	 * Output:         cell_ip, 存储机器ip
	 * Output:         N/A
	 * Return:         0, 成功； <0, 错误
	 * Others:         N/A
	 **************************************************/
	int Redirect(unsigned t_id, unsigned c_id
			, const std::string& key, unsigned& cell_ip);

	int Redirect(unsigned t_id, unsigned c_id
			, unsigned uin, unsigned& cell_ip);

	/*************************************************
	 * Function:       Set
	 * Description:    添加、修改记录
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 key, 记录的键
	 * Output:         value, 记录的值
	 * Output:         N/A
	 * Return:         0, 成功； <0, 错误
	 * Others:         N/A
	 **************************************************/
	int Set(unsigned t_id, unsigned c_id
			, const std::string& key, const std::string& value);

	int Set(unsigned t_id, unsigned c_id
			, unsigned uin, const std::string& value);

	/*************************************************
	 * Function:       Del
	 * Description:    删除记录
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 key, 记录的键
	 * Output:         N/A
	 * Return:         0, 成功； <0, 错误
	 * Others:         N/A
	 **************************************************/
	int Del(unsigned t_id, unsigned c_id
			, const std::string& key);

	int Del(unsigned t_id, unsigned c_id
			, unsigned uin);


	/*************************************************
	 * Function:       Get
	 * Description:    添加、修改记录
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 key, 记录的键
	 * Output:         value, 记录的值
	 * Return:         0, 成功； <0, 错误
	 * Others:         N/A
	 **************************************************/
	int Get(unsigned t_id, unsigned c_id
			, const std::string& key, std::string& value);	 

	int Get(unsigned t_id, unsigned c_id
			, unsigned uin, std::string& value);	 

    /*************************************************
	 * Function:       GetMuti
	 * Description:   批量获得记录
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 keyseq, 记录的键列表
	 * Output:         record_map, 记录列表
	 * Return:         0, 所有记录都有返回 需要检查每个返回值
	 *                 <0, 所有记录都没有返回
	 *                 
	 * Others:         N/A
	 **************************************************/
	int GetMuti(unsigned t_id, unsigned c_id
			, std::vector<std::string>& key_vec, std::map<std::string,RspRecord>& record_map);	
         
    /*************************************************
	 * Function:       SetMuti
	 * Description:   批量修改记录
	 * Input:          t_id, 分配给业务的表ID
	 *                 c_id, 对应该表ID的校验ID
	 *                 record_value, 记录的键值列表
	 * Output:         rspset_map, 记录修改返回码
	 * Return:         0, 所有记录都有返回，需要检查每个返回值 
	 *                 <0, 所有记录都没有返回
	 * Others:         N/A
	 **************************************************/
	int SetMuti(unsigned t_id, unsigned c_id
			, std::vector<RecordValue>& record_value, std::map<std::string,RspSetRecord>& rspset_map);	
};

}}

#endif//_TDB_HANDLE_H_
