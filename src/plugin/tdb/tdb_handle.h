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
 * hand_ip : ���������ip
 * route_ip : ·��server��ip
 * cell_ip : ʵ�ʴ洢���ݵ�ip 
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
	
	//���ò�����ip�Ķ���ʱ��
	void SetFreezeLinkSecs(int iFreezeSecs);
	//�����µ�ַ	
	void AddAddr(const std::string& hand_ip);	
	
	void SetTime(unsigned send_time, unsigned recv_time/*ms*/);
	void GetTime(unsigned& send_time, unsigned& recv_time/*ms*/);
	void SetAddr(const std::string& hand_ip);
	std::string GetAddr();

	void SetRouteAddr(const std::string& rout_ip);
	std::string GetRouteAddr();
	
	/*************************************************
	 * Function:       Redirect
	 * Description:    ����key����ȡ��Ӧ�洢������ip
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 key, ��¼�ļ�
	 * Output:         cell_ip, �洢����ip
	 * Output:         N/A
	 * Return:         0, �ɹ��� <0, ����
	 * Others:         N/A
	 **************************************************/
	int Redirect(unsigned t_id, unsigned c_id
			, const std::string& key, unsigned& cell_ip);

	int Redirect(unsigned t_id, unsigned c_id
			, unsigned uin, unsigned& cell_ip);

	/*************************************************
	 * Function:       Set
	 * Description:    ��ӡ��޸ļ�¼
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 key, ��¼�ļ�
	 * Output:         value, ��¼��ֵ
	 * Output:         N/A
	 * Return:         0, �ɹ��� <0, ����
	 * Others:         N/A
	 **************************************************/
	int Set(unsigned t_id, unsigned c_id
			, const std::string& key, const std::string& value);

	int Set(unsigned t_id, unsigned c_id
			, unsigned uin, const std::string& value);

	/*************************************************
	 * Function:       Del
	 * Description:    ɾ����¼
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 key, ��¼�ļ�
	 * Output:         N/A
	 * Return:         0, �ɹ��� <0, ����
	 * Others:         N/A
	 **************************************************/
	int Del(unsigned t_id, unsigned c_id
			, const std::string& key);

	int Del(unsigned t_id, unsigned c_id
			, unsigned uin);


	/*************************************************
	 * Function:       Get
	 * Description:    ��ӡ��޸ļ�¼
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 key, ��¼�ļ�
	 * Output:         value, ��¼��ֵ
	 * Return:         0, �ɹ��� <0, ����
	 * Others:         N/A
	 **************************************************/
	int Get(unsigned t_id, unsigned c_id
			, const std::string& key, std::string& value);	 

	int Get(unsigned t_id, unsigned c_id
			, unsigned uin, std::string& value);	 

    /*************************************************
	 * Function:       GetMuti
	 * Description:   ������ü�¼
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 keyseq, ��¼�ļ��б�
	 * Output:         record_map, ��¼�б�
	 * Return:         0, ���м�¼���з��� ��Ҫ���ÿ������ֵ
	 *                 <0, ���м�¼��û�з���
	 *                 
	 * Others:         N/A
	 **************************************************/
	int GetMuti(unsigned t_id, unsigned c_id
			, std::vector<std::string>& key_vec, std::map<std::string,RspRecord>& record_map);	
         
    /*************************************************
	 * Function:       SetMuti
	 * Description:   �����޸ļ�¼
	 * Input:          t_id, �����ҵ��ı�ID
	 *                 c_id, ��Ӧ�ñ�ID��У��ID
	 *                 record_value, ��¼�ļ�ֵ�б�
	 * Output:         rspset_map, ��¼�޸ķ�����
	 * Return:         0, ���м�¼���з��أ���Ҫ���ÿ������ֵ 
	 *                 <0, ���м�¼��û�з���
	 * Others:         N/A
	 **************************************************/
	int SetMuti(unsigned t_id, unsigned c_id
			, std::vector<RecordValue>& record_value, std::map<std::string,RspSetRecord>& rspset_map);	
};

}}

#endif//_TDB_HANDLE_H_
