/*
 * =====================================================================================
 *
 *       Filename:  key_checker.cc
 *
 *    Description:  检查key是否属于此次迁移范围
 *
 *        Version:  1.0
 *        Created:  11/20/2010 11:30:03 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include "key_checker.h"
#include "log.h"
#include <stdlib.h>
using namespace std;

bool CKeyChecker::IsKeyInFilter(const char *key,const int len)
{
	set<string>::iterator it;
	uint64_t key_tmp = 0; 
	string nodename;
	switch (_keytype)
	{
		//和agent及ttc约定，intkey统一使用8字节uint来进行hash
		case 1:
		case 2:
			if (len==4)
			{
				key_tmp = *(uint32_t*)key;
					nodename = _hashSelector.Select(((char *)&key_tmp),sizeof(key_tmp));
				break;
			}
			else if (len == 8)
			{
				nodename = _hashSelector.Select(key,len);
				break;
			}
			else
			{
				log_crit("error key len:%d type:%d",len,_keytype);
				exit(-1);
			}
		case 3:
		case 4:
		case 5:
			nodename = _hashSelector.Select(key,len);
			break;
		default:
			log_crit("error key type:%d",_keytype);
			exit(-1);
	}

	log_debug("nodename:%s key:%d",nodename.c_str(),(int)key_tmp);
	it = _filter->find(nodename);
	if (it != _filter->end())
		return true;
	return false;
}
