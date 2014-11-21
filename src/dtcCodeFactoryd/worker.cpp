#include "worker.h"
#include <sstream>
#include <stdlib.h>
#include "log.h"
#include "mysql_manager.h"
#include "http_agent_error_code.h"
#include "http_common.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>

extern MySqlManager *g_manager;

MsgParser::MsgParser(){

}
MsgParser::~MsgParser(){

}

/*
 * content: {"bid":50, "cmd":1, "agentIp":"1.1.1.1", "agentPort":"10001", "accessKey":"0000000184d9cfc2f395ce883a41d7ffc1bbcf4e"}
 */
int MsgParser::Parse(const string& content, int& bid, int& cmd, string& ip, string& port, string& token, string& errMsg){
	int ret = 0;
	log_info("content: %s", content.c_str());
	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(content.c_str(), root)){
		ret = ERROR_CODEFACTORY_PARSE_JSON_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("parse Json failed, errCode:%d, content:%s", ret, content.c_str());
		return ERROR_CODEFACTORY_PARSE_JSON_FAIL;
	}
	bid = root["bid"].asInt();
	cmd = root["cmd"].asInt();
	ip = root["agentIp"].asString();
	port = root["agentPort"].asString();
	token = root["accessKey"].asString();
	if(bid == 0 || cmd == 0 || ip.empty() || port.empty() || token.empty()){
		ret = ERROR_CODEFACTORY_JSON_REQUEST_EMPTY;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("empty json request! errCode:%d, bid:%d, cmd:%d, ip:%s, port:%s, accessKey:%s", ret, bid, cmd, ip.c_str(), port.c_str(), token.c_str());
		return ERROR_CODEFACTORY_JSON_REQUEST_EMPTY;
	}
	log_info("bid: %d, cmd: %d, agentIp: %s, agentPort: %s, accessKey: %s", bid, cmd, ip.c_str(), port.c_str(), token.c_str());
	return ret;
}

Worker::Worker(){

}
Worker::~Worker(){

}

/*
 * 查询db，获取 bid 对应的 table.conf 字段
 */
int Worker::GetTableConf(const int& bid, string& tableconf, string& errMsg){
	int ret = 0;
	if(bid == 0){
		ret = ERROR_CODEFACTORY_EMPTY_BID;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("worker get empty bid, errCode: %d, errMsg: %s", ret, errMsg.c_str());
		return ret;
	}
	string sql1, sql2;
	char sql[256];
	memset(sql, 0, sizeof(sql));
	sql1 = "select table_conf from dtc_instance where business_id=";
	sql2 = "limit 1;";
	sprintf(sql, "%s %d %s", sql1.c_str(), bid, sql2.c_str());
	log_info("sql: %s", sql);
	if(sql == NULL){
		ret = ERROR_CODEFACTORY_QUERY_SQL_EMPTY;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("query sql is empty, errCode:%d, errMsg: %s, sql: %s", ret, errMsg.c_str(), sql);
		return ret;
	}
	// 查询db，获取 bid 对应的 table.conf 字段
	ret = g_manager->ExecuteSql(sql, tableconf, errMsg);
	if(ret < 0){
		ret = ERROR_CODEFACTORY_EXECUTE_SQL_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("execute mysql failed. errCode:%d, errMsg: %s, sql is %s", ret, errMsg.c_str(), sql);
		return ret;
	}
	if(tableconf == ""){
		ret = ERROR_CODEFACTORY_EXECUTE_SQL_RESULT_EMPTY;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("execute mysql get tableconf empty. errCode:%d, errMsg: %s, tableconf is %s", ret, errMsg.c_str(), tableconf.c_str());
		return ret;
	}

	return ret;
}
/*
 * 1,根据每个  bid 把读取到的 table.conf 字符串 写入相应bid文件夹下的 table.conf文件中
 * 2, 并根据请求 json 中的 cmd 创建子目录
 * 		cmd:1, 创建 cplus 子目录
 * 		cmd:2, 创建 java 子目录
 * 		cmd:3, 创建 php 子目录
 * 3,调用 php 程序，分别生成 每个业务所用表名 的 模版代码
 */
int Worker::MakeTplCode(
		const string& tableconf,
		const int& cmd,
		const int& bid,
		const string& ip,
		const string& port,
		const string& token,
		string& tplCodeDir,
		string& errMsg)
{
	extern std::string g_strGenPath;
	extern std::string g_versioninfo;
	extern std::string g_phpbin;
	int ret = 0;
	fstream conf;
	stringstream sstmp;
	string tableConfDir, cfgdir, sdir, agent_ip, agent_port, access_token, stplCode, tgzdir;

	char scode[1024];
	memset(scode, 0, sizeof(scode));
	string str1 = "/export/servers/dtcCodeFactoryd_";
	string str2 = "_CentOS64/PhpCodeFactory/code/codeGenerator.php";
	sprintf(scode, "%s%s%s", str1.c_str(), g_versioninfo.c_str(), str2.c_str());
	log_info("scode:%s", scode);
	char commond[1024];
	memset(commond, 0, sizeof(commond));
	char tar_cmd[256];
	memset(tar_cmd, 0, sizeof(tar_cmd));
	switch(cmd%10){
	case 1:
	{
		sstmp << g_strGenPath << "/" << bid;
		sstmp >> sdir;
		// 判断是否已经创建了 bid 子目录，没有则创建， 有就跳过
		if(NULL == opendir(sdir.c_str())){
			ret = mkdir(sdir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir sdir: %s fail, ret: %d", sdir.c_str(), ret);
			}
			tplCodeDir = sdir + "/cplus/";
			ret = mkdir(tplCodeDir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
			}
		}else{
			tplCodeDir = sdir + "/cplus/";
			if(NULL == opendir(tplCodeDir.c_str())){
				ret = mkdir(tplCodeDir.c_str(), 0755);
				if(0 != ret){
					log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
				}
			}
		}
		tgzdir = tplCodeDir;
		sstmp.str("");
		sstmp.clear();
		sstmp << "--config=\"" << g_strGenPath << "/" << bid << "/cplus/table.conf\"";
		sstmp >> tableConfDir;

		sstmp.str("");
		sstmp.clear();
		sstmp << g_strGenPath << "/" << bid << "/cplus/table.conf";
		sstmp >> cfgdir;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_ip=\"" << ip.c_str() << "\"";
		sstmp >> agent_ip;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_port=\"" << port.c_str() << "\"";
		sstmp >> agent_port;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--access_token=\"" << token.c_str() << "\"";
		sstmp >> access_token;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--cplus_dir=\"" << tplCodeDir << "\"";
		sstmp >> stplCode;

		sprintf(commond, "%s %s %s %s %s %s %s", g_phpbin.c_str(), scode, tableConfDir.c_str(),
				agent_ip.c_str(), agent_port.c_str(), access_token.c_str(), stplCode.c_str());
		if(cmd/10 == 0){
			string s1="cd";
			string s2="&& mkdir tpl_cplus && cp *.cpp *.h tpl_cplus && zip -r tpl_cplus.zip tpl_cplus";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			tplCodeDir += "tpl_cplus.zip";
		}else{
			string s1="cd";
			string s2="&& mkdir tpl_cplus && cp *.cpp *.h tpl_cplus && tar -zcvf tpl_cplus.tgz tpl_cplus";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			tplCodeDir += "tpl_cplus.tgz";
		}
		log_info("php commond for make cplus tplCode: %s, tar_cmd: %s", commond, tar_cmd);

		if(commond == NULL){
			ret = ERROR_CODEFACTORY_EMPTY_CPLUS_COMMOND;
			errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
			log_error("sprintf get cplus commond empty. errCode: %d, errMsg: %s", ret, errMsg.c_str());
			return ret;
		}
		break;
	}
	case 2:
	{
		sstmp << g_strGenPath << "/" << bid;
		sstmp >> sdir;
		// 判断是否已经创建了 bid 子目录，没有则创建， 有就跳过
		if(NULL == opendir(sdir.c_str())){
			ret = mkdir(sdir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir sdir: %s fail, ret: %d", sdir.c_str(), ret);
			}
			tplCodeDir = sdir + "/java/";
			ret = mkdir(tplCodeDir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
			}
		}else{
			tplCodeDir = sdir + "/java/";
			if(NULL == opendir(tplCodeDir.c_str())){
				ret = mkdir(tplCodeDir.c_str(), 0755);
				if(0 != ret){
					log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
				}
			}
		}
		tgzdir = tplCodeDir;
		sstmp.str("");
		sstmp.clear();
		sstmp << "--config=\"" << g_strGenPath << "/" << bid << "/java/table.conf\"";
		sstmp >> tableConfDir;

		sstmp.str("");
		sstmp.clear();
		sstmp << g_strGenPath << "/" << bid << "/java/table.conf";
		sstmp >> cfgdir;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_ip=\"" << ip.c_str() << "\"";
		sstmp >> agent_ip;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_port=\"" << port.c_str() << "\"";
		sstmp >> agent_port;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--access_token=\"" << token.c_str() << "\"";
		sstmp >> access_token;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--java_dir=\"" << tplCodeDir << "\"";
		sstmp >> stplCode;

		sprintf(commond, "%s %s %s %s %s %s %s", g_phpbin.c_str(), scode, tableConfDir.c_str(),
				agent_ip.c_str(), agent_port.c_str(), access_token.c_str(), stplCode.c_str());
		if(cmd/10 == 0){
			string s1="cd";
			string s2="&& mkdir tpl_java && cp *.java tpl_java && zip -r tpl_java.zip tpl_java";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			log_info("php commond for make java tplCode: %s, tar_cmd: %s", commond, tar_cmd);
			tplCodeDir += "tpl_java.zip";
		}else{
			string s1="cd";
			string s2="&& mkdir tpl_java && cp *.java tpl_java && tar -zcvf tpl_java.tgz tpl_java";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			log_info("php commond for make java tplCode: %s, tar_cmd: %s", commond, tar_cmd);
			tplCodeDir += "tpl_java.tgz";
		}

		if(commond == NULL){
			ret = ERROR_CODEFACTORY_EMPTY_JAVA_COMMOND;
			errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
			log_error("sprintf get java commond empty. errCode: %d, errMsg: %s", ret, errMsg.c_str());
			return ret;
		}
		break;
	}
	case 3:
	{
		sstmp << g_strGenPath << "/" << bid;
		sstmp >> sdir;
		// 判断是否已经创建了 bid 子目录，没有则创建， 有就跳过
		if(NULL == opendir(sdir.c_str())){
			ret = mkdir(sdir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir sdir: %s fail, ret: %d", sdir.c_str(), ret);
			}
			tplCodeDir = sdir + "/php/";
			ret = mkdir(tplCodeDir.c_str(), 0755);
			if(0 != ret){
				log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
			}
		}else{
			tplCodeDir = sdir + "/php/";
			if(NULL == opendir(tplCodeDir.c_str())){
				ret = mkdir(tplCodeDir.c_str(), 0755);
				if(0 != ret){
					log_error("mkdir tplCodeDir: %s fail, ret: %d", tplCodeDir.c_str(), ret);
				}
			}
		}
		tgzdir = tplCodeDir;
		sstmp.str("");
		sstmp.clear();
		sstmp << "--config=\"" << g_strGenPath << "/" << bid << "/php/table.conf\"";
		sstmp >> tableConfDir;

		sstmp.str("");
		sstmp.clear();
		sstmp << g_strGenPath << "/" << bid << "/php/table.conf";
		sstmp >> cfgdir;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_ip=\"" << ip.c_str() << "\"";
		sstmp >> agent_ip;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--agent_port=\"" << port.c_str() << "\"";
		sstmp >> agent_port;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--access_token=\"" << token.c_str() << "\"";
		sstmp >> access_token;

		sstmp.str("");
		sstmp.clear();
		sstmp << "--php_dir=\"" << tplCodeDir << "\"";
		sstmp >> stplCode;

		sprintf(commond, "%s %s %s %s %s %s %s", g_phpbin.c_str(), scode, tableConfDir.c_str(),
				agent_ip.c_str(), agent_port.c_str(), access_token.c_str(), stplCode.c_str());
		if(cmd/10 == 0){
			string s1="cd";
			string s2="&& mkdir tpl_php && cp *.php tpl_php && zip -r tpl_php.zip tpl_php";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			log_info("php commond for make php tplCode: %s, tar_cmd: %s", commond, tar_cmd);
			tplCodeDir += "tpl_php.zip";
		}else{
			string s1="cd";
			string s2="&& mkdir tpl_php && cp *.php tpl_php && tar -zcvf tpl_php.tgz tpl_php";
			sprintf(tar_cmd, "%s %s %s", s1.c_str(), tplCodeDir.c_str(), s2.c_str());
			log_info("php commond for make php tplCode: %s, tar_cmd: %s", commond, tar_cmd);
			tplCodeDir += "tpl_php.tgz";
		}
		if(commond == NULL){
			ret = ERROR_CODEFACTORY_EMPTY_PHP_COMMOND;
			errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
			log_error("sprintf get php commond empty. errCode: %d, errMsg: %s", ret, errMsg.c_str());
			return ret;
		}
		break;
	}
	default:
		ret = ERROR_CODEFACTORY_INVALID_REQUEST_CMD;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("invalid request cmd, cmd: %d", cmd);
		return ret;
	}
	log_info("cfgdir: %s", cfgdir.c_str());
	conf.open(cfgdir.c_str(), std::fstream::out);
	conf << tableconf.c_str();
	conf.close();
	if(access(cfgdir.c_str(), F_OK) < 0){
		ret = ERROR_CODEFACTORY_WRITE_TABLECONF_FAIL;
		errMsg = CErrorCode::GetInstance()->GetErrorMessage(ret);
		log_error("write table.conf fail. errCode: %d, errMsg: %s", ret, errMsg.c_str());
	}
	// 调用 php 程序，分别生成 每个业务所用表名 的 模版代码
	system(commond);

	char rm_cmd[512];
	memset(rm_cmd, 0, sizeof(rm_cmd));
	switch(cmd%10){
	case 1:
	{
		string s1 = tgzdir + "tpl_cplus/";
		if(NULL != opendir(s1.c_str())){
			string scd = "cd";
			string stmp = "&& rm -fr tpl_cplus*";
			sprintf(rm_cmd, "%s %s %s", scd.c_str(), tgzdir.c_str(), stmp.c_str());
		}
		break;
	}
	case 2:
	{
		string s1 = tgzdir + "tpl_java/";
		if(NULL != opendir(s1.c_str())){
			string scd = "cd";
			string stmp = "&& rm -fr tpl_java*";
			sprintf(rm_cmd, "%s %s %s", scd.c_str(), tgzdir.c_str(), stmp.c_str());
		}
		break;
	}
	case 3:
	{
		string s1 = tgzdir + "tpl_php/";
		if(NULL != opendir(s1.c_str())){
			string scd = "cd";
			string stmp = "&& rm -fr tpl_php*";
			sprintf(rm_cmd, "%s %s %s", scd.c_str(), tgzdir.c_str(), stmp.c_str());
		}
		break;
	}
	default:
		break;
	}
	// remove original tar dir and tgz
	system(rm_cmd);
	// tar
	system(tar_cmd);

	return ret;
}
