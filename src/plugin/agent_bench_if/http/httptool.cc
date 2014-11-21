/*
 * httptool.cc
 *
 *  Created on: 2014年8月18日
 *      Author: p_xiangxli
 */
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <stdio.h>

#include "log.h"
#include "json/json.h"
#include "http_plugin_request_msg.h"
#include "http_plugin_response_msg.h"
#include "ttcint.h"
#include "tabledef.h"
#include "config.h"
#include "httptool.h"

extern CConfig *gConfig;
std::map<std::string, NCServer*> HttpTool::serverGroup;

int HttpTool::setKeyType(NCServer* server, const int keyType) {
	switch(keyType) {
		case DField::Signed:
		case DField::Unsigned:
			return server->IntKey();
			break;
		case DField::String:
		case DField::Binary:
			return server->StringKey();
			break;
		default:
			return -1;
	}
}

int HttpTool::MakeJsonField(NCResult* result, const int ftype, const char* fname, Json::Value& packet) {
	const CValue *v;
	int fid = result->FieldId(fname);
	if(fid==0 && !(result->result->FieldPresent(0)))
		v = result->ResultKey();
	else
		v = result->result->FieldValue(fid);

	switch(ftype){
	case DField::Signed:
	case DField::Unsigned:
		packet[fname] = (int) v->s64;
		break;
	case DField::Float:
		packet[fname] = v->flt;;
		break;
	case DField::String:
	case DField::Binary:
		packet[fname] = v->str.ptr;
		break;
	}
	return 0;
}

int HttpTool::Parse(const std::string body, RequestBody& reqBody) {
	Json::Value root;
	Json::Reader reader;
	if(!reader.parse(body.c_str(), root))
	{
		log_error("parse Json failed, strRsp:%s", body.c_str());
		return -1;
	}
	reqBody.acsToken = root["token"].asString();
	if(""==reqBody.acsToken) {
		log_error("lack of token");
		return -1;
	}
	reqBody.tableName = root["table"].asString();
	reqBody.keyType = root["keyType"].asInt();
	if(""==reqBody.tableName || 0==reqBody.keyType ) {
		log_error("param error, strRsp:%s", body.c_str());
		return -1;
	}

	switch(reqBody.keyType) {
	case DField::Signed:
	case DField::Unsigned:
		reqBody.intKey = root["key"].asInt();
		break;
	case DField::String:
	case DField::Binary:
		reqBody.strKey = root["key"].asString();
		break;
	}

	return 0;
}

NCServer* HttpTool::getNCServer(const RequestBody reqBody) {
	std::map<std::string, NCServer*>::iterator it = serverGroup.find(reqBody.acsToken);
	if(serverGroup.end() == it) {
		const char* port;
		port = gConfig->GetStrVal("port", reqBody.acsToken.c_str());
		if(NULL==port) {
			log_error("get port error");
			return NULL;
		}
		log_debug("port=%s", port);

		NCServer* server = new NCServer();
		int ret = HttpTool::setKeyType(server,reqBody.keyType);
		ret = server->SetTableName(reqBody.tableName.c_str());
		ret = server->SetAddress("127.0.0.1", port);
		ret = server->SetAccessKey(reqBody.acsToken.c_str());
		server->SetMTimeout(5000);
		server->INC();
		if(ret != 0) {
			log_error("set Server error, ret=%d", ret);
			return NULL;
		}
		ret = server->Ping();
		if(ret != 0) {
			log_error("ping error, ret=%d", ret);
			return NULL;
		}
		serverGroup.insert(make_pair(reqBody.acsToken,server));
		log_debug("insert server ok!");
		return server;
	}
	else {
		return it->second;
	}
}

int HttpTool::doRequest(NCServer* server, const RequestBody reqBody, char** send_buf, int* send_len) {
	int ret = 0;
	CTableDefinition* tdef = server->tdef;
	log_debug("tdef=%p",tdef);
	NCRequest request(server, DRequest::Get);

	//Set Key
	switch(reqBody.keyType) {
	case DField::Signed:
	case DField::Unsigned:
		ret = request.SetKey(reqBody.intKey);
		break;
	case DField::String:
	case DField::Binary:
		ret = request.SetKey(reqBody.strKey.c_str(), reqBody.strKey.size());
		break;
	}
	if(ret != 0) {
		log_error("set Key error!");
		return -1;
	}

	//Set Need
	for(int i=0; i <= tdef->NumFields(); i++) {
		ret = request.Need(tdef->FieldName(i), 0);
		if(ret != 0) {
			log_error("set Need error!");
			return -1;
		}
	}

	// Execute
	NCResult* result;
	result = request.Execute();
	if(NULL == result) {
		log_error("Execute error!");
		return -1;
	}
	if(result->ResultCode() < 0) {
		log_error("Execute error! ret=%d", result->ResultCode());
		return -1;
	}

	// Empty Result
	if(NULL == result->result) {
		log_info("Empty result!");
		CHttpPluginResponseMsg encoder;
		encoder.SetCode(200);
		encoder.SetHttpVersion(1);
		encoder.SetKeepAlive(false);

		Json::FastWriter writer;
		Json::Value packet;
		packet["retCode"] = 1;
		packet["errMsg"] = "empty result!";

		std::string respBody = writer.write(packet);
		encoder.SetBody(respBody);
		std::string response = encoder.Encode();
		char* send = (char*) malloc(response.size());
		memcpy(send, response.c_str(), response.size());
		*send_buf = send;
		*send_len = response.size();
		return 0;
	}

	// Parse Result Json
	Json::FastWriter writer;
	Json::Value packet;
	packet["retCode"] = 0;
	int numRows = result->result->TotalRows();
	log_debug("total rows=%d",numRows);
	for(int i=0; i<numRows; i++) {
		ret = result->result->DecodeRow();
		if(ret != 0) {
			log_error("fetch error! ret=%d", ret);
			return -1;
		}

		// get one row
		Json::Value row;
		for(int j=0; j<=tdef->NumFields(); j++) {
			int ftype = tdef->FieldType(j);
			const char* fname = tdef->FieldName(j);
			HttpTool::MakeJsonField(result, ftype, fname, row);
		}
		packet["values"].append(row);
	}
	std::string respBody = writer.write(packet);
	log_debug("respBody=%s", respBody.c_str());

	// Make Http Body
	CHttpPluginResponseMsg encoder;
	encoder.SetCode(200);
	encoder.SetHttpVersion(1);
	encoder.SetKeepAlive(false);
	encoder.SetBody(respBody);
	std::string response = encoder.Encode();
	char* send = (char*) malloc(response.size());
	memcpy(send, response.c_str(), response.size());
	*send_buf = send;
	*send_len = response.size();

	return 0;
}

void HttpTool::doError(char **send_buf, int *send_len) {
	CHttpPluginResponseMsg encoder;
	encoder.SetCode(200);
	encoder.SetHttpVersion(1);
	encoder.SetKeepAlive(false);

	Json::FastWriter writer;
	Json::Value packet;

	packet["retCode"] = -1;
	packet["errMsg"] = "http error!";

	std::string respBody = writer.write(packet);
	encoder.SetBody(respBody);
	std::string response = encoder.Encode();
	char* send = (char*) malloc(response.size());
	memcpy(send, response.c_str(), response.size());
	*send_buf = send;
	*send_len = response.size();
}

