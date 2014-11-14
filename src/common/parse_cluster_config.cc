/*
 * =====================================================================================
 *
 *       Filename:  parse_cluster_config.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/07/2010 10:32:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  foryzhou (fory), foryzhou@tencent.com
 *        Company:  Tencent, China
 *
 * =====================================================================================
 */
#include <errno.h>
#include <stdio.h>
#include "config.h"
#include <set>

#include "parse_cluster_config.h"
#include "MarkupSTL.h"
#include "log.h"
#include "sockaddr.h"

using namespace std;

extern int errno;
extern CConfig *gConfig;    

	namespace ClusterConfig{
		bool ParseClusterConfig(std::vector<ClusterNode> *result, const char *buf, int len)
		{
			log_debug("%.*s", len, buf);
			//配置中不允许有相同servername的节点出现，本set用于检查重复servername
			std::set<std::string> filter;
			pair<set<std::string>::iterator,bool> ret;
			const char * bindaddr = gConfig->GetStrVal ("cache", "BindAddr");
			if (bindaddr == NULL)
			{
				log_error("BindAddr is empty");
				return false;
			}
			string myaddr = bindaddr;
			if (myaddr.substr(0,3)==string("eth")||myaddr.substr(0,2)==string("lo"))
			{
				CSocketAddress addr;
				const char * ret = addr.SetAddress(myaddr.c_str(), (const char *)0);
				if (ret != NULL)
				{
					log_error("ttc bind interface %s unknown:%s",myaddr.c_str(),ret);
					return false;
				}
				log_debug("ttc bind interface %s be resolved to %s",
						myaddr.c_str(), addr.Name());
				myaddr = addr.Name();
			}
			string xmldoc(buf,len);
			CMarkupSTL xml;
			if(!xml.SetDoc(xmldoc.c_str()))
			{
				log_error("parse config file error");
				return false;
			}
			xml.ResetMainPos();

			while (xml.FindChildElem("server"))
			{
				if (xml.IntoElem())
				{
					struct ClusterNode node;
					node.name = xml.GetAttrib("name");
					node.addr = xml.GetAttrib("address");
					if (node.name.empty()||node.addr.empty())
					{
						log_error("name:%s addr:%s may error\n",node.name.c_str(),node.addr.c_str());
						return false;
					}
					if (myaddr == node.addr)
						node.self = true;
					else
						node.self = false;
					ret = filter.insert(node.name);
					if (ret.second == false)
					{
						log_error("server name duplicate. name:%s addr:%s \n",node.name.c_str(),node.addr.c_str());
						return false;
					}
					ret = filter.insert(node.addr);
					if (ret.second == false)
					{
						log_error("server address duplicate. name:%s addr:%s \n",node.name.c_str(),node.addr.c_str());
						return false;
					}
					result->push_back(node);
					xml.OutOfElem();
				}
			}
			if (result->empty())
				log_notice("ClusterConfig is empty,ttc runing in normal mode");
			return true;

		}

		//读取配置文件
		bool ParseClusterConfig(std::vector<ClusterNode> *result)
		{
			bool bResult = false;
			FILE* fp = fopen(CLUSTER_CONFIG_FILE, "rb" );
			if (fp == NULL)
			{
				log_error("load %s failed:%m",CLUSTER_CONFIG_FILE);
				return false;
			}

			// Determine file length
			fseek( fp, 0L, SEEK_END );
			int nFileLen = ftell(fp);
			fseek( fp, 0L, SEEK_SET );

			// Load string
			allocator<char> mem;
			allocator<char>::pointer pBuffer = mem.allocate(nFileLen+1, NULL);
			if ( fread( pBuffer, nFileLen, 1, fp ) == 1 )
			{
				pBuffer[nFileLen] = '\0';
				bResult = ParseClusterConfig(result,pBuffer,nFileLen);
			}
			fclose(fp);
			mem.deallocate(pBuffer,1);
			return bResult;
		}

		//保存vector到配置文件
		bool SaveClusterConfig(std::vector<ClusterNode> * result)
		{
			CMarkupSTL xml;
			xml.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
			xml.AddElem("serverlist");
			xml.IntoElem();

			std::vector<ClusterNode>::iterator it;
			for (it=result->begin(); it<result->end(); it++)
			{
				xml.AddElem("server");
				xml.AddAttrib("name",it->name.c_str());
				xml.AddAttrib("address",it->addr.c_str());
			}
			return xml.Save(CLUSTER_CONFIG_FILE);
		}

		//检查是否有配置文件，如果没有则生成配置文件
		bool CheckAndCreate(const char * filename)
		{
			if (filename == NULL)
				filename = CLUSTER_CONFIG_FILE;

			if(access(filename, F_OK))
			{
				//配置文件不存在,创建配置文件
				if (errno == ENOENT)
				{
					log_notice("%s didn't exist,create it.",filename);
					CMarkupSTL xml;
					xml.SetDoc("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
					xml.AddElem("serverlist");
					xml.IntoElem();
					return xml.Save(filename);

				}
				log_error("access %s error:%m\n",filename);
				return false;
			}
			else
				return true;
		}


		bool ChangeNodeAddress(std::string servername, std::string newaddress)
		{
			CMarkupSTL xml;
			if (!xml.Load(CLUSTER_CONFIG_FILE))
			{
				log_error("load config file: %s error",CLUSTER_CONFIG_FILE);
				return false;
			}
			xml.ResetMainPos();
			while (xml.FindChildElem("server"))
			{
				if (!xml.IntoElem())
				{
					log_error("xml.IntoElem() error");
					return false;
				}

				string name = xml.GetAttrib("name");
				string address = xml.GetAttrib("address");
				if (address == newaddress)
				{
					log_error("new address:%s is exist in Cluster Config",newaddress.c_str());
					return false;
				}

				if (!xml.OutOfElem())
				{
					log_error("xml.OutElem() error");
					return false;
				}

			}

			xml.ResetMainPos();
			while ( xml.FindChildElem("server") )
			{
				if (!xml.IntoElem())
				{
					log_error("xml.IntoElem() error");
					return false;
				}

				string name = xml.GetAttrib("name");
				if (name != servername)
				{
					if (!xml.OutOfElem())
					{
						log_error("xml.OutElem() error");
						return false;
					}
					continue;
				}

				string oldaddress = xml.GetAttrib("address");
				if (oldaddress == newaddress)
				{
					log_error("server name:%s ip address:%s not change",
							servername.c_str(),newaddress.c_str());
					return false;
				}

				if (!xml.SetAttrib("address",newaddress.c_str()))
				{
					log_error("change address error.server name: %s old address: %s new address: %s",
							servername.c_str(),oldaddress.c_str(),newaddress.c_str());
					return false;
				}
				return xml.Save(CLUSTER_CONFIG_FILE);
			}
			log_error("servername: \"%s\" not found",servername.c_str());
			return false;
		}
	}
