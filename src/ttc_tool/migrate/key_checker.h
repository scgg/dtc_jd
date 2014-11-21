/*
 * =====================================================================================
 *
 *       Filename:  key_checker.h
 *
 *    Description:  检查key是否属于此次迁移范围
 *
 *        Version:  1.0
 *        Created:  11/20/2010 11:29:29 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#ifndef _KEY_CHECKER_H_
#define _KEY_CHECKER_H_
#include <set>
#include <string>
#include "consistent_hash_selector.h"

class CKeyChecker
{
		public:
				CKeyChecker(std::set<std::string>* filter,std::set<std::string>* nodelist,int keytype)
				{
						_filter = filter;
						_keytype=keytype;
						for(std::set<std::string>::const_iterator i = nodelist->begin();
										i != nodelist->end(); ++i)
						{       
								_hashSelector.AddNode(i->c_str());
						}

				};
				~CKeyChecker()
				{
				};
				bool IsKeyInFilter(const char *key,const int len);
		private:
				CConsistentHashSelector _hashSelector;
				std::set<std::string> * _filter;
				int _keytype;
};

#endif


