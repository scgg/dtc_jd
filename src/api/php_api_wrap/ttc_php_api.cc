#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "ttc_php_api.h"
#include "ttcapi.h"

#define CAST(type, var) type* var = (type *)ptr
#define NAME(var, p) snprintf(conn->var, sizeof(conn->var), "%s", p)

#define FLAG_SET_TAB 0x1U
#define FLAG_KEY_TYPE 0x2U

#ifdef DEBUG_M
#define MALLOC malloc
#define REALLOC realloc
#define FREE free
#else
#define MALLOC emalloc
#define REALLOC erealloc
#define FREE efree
#endif

using namespace TTC;

/*
mysql table definition should like:

create table `t_data`(
`key` char(255) NOT NULL,
`flags` int NOT NULL DEFAULT 0,
`expire_time` int NOT NULL DEFAULT 0,
`len` int NOT NULL DEFAULT 0,
`data` blob NOT NULL,
primary key(`key`)
);
*/

static int key_type_map[]={KeyTypeInt, KeyTypeString};

typedef struct _ttc_conn{
	TTC::Server server;
	unsigned int flags;
	int mergeValue;
	char tab_name[64];
	int key_type;
	char key_name[32];
	char len_name[32];
	char val_name[32];
	char flag_name[32];
	char exp_name[32];
	char errmsg[200];
	
	_ttc_conn():flags(0)
	{
		mergeValue = 0;
		snprintf(tab_name, sizeof(tab_name), "t_data");
		key_type = TTC_KEY_TYPE_STRING;
		snprintf(key_name, sizeof(key_name), "key");
		snprintf(len_name, sizeof(len_name), "len");
		snprintf(val_name, sizeof(val_name), "data");
		snprintf(flag_name, sizeof(flag_name), "flags");
		snprintf(exp_name, sizeof(exp_name), "expire_time");
		
		memset(errmsg, 0, sizeof(errmsg));
	}
}TTC_CONN;

const char* api_ttc_strerror(void* ptr)
{
	CAST(TTC_CONN, conn);
	return conn->errmsg;
}

static int connect_init(TTC_CONN* conn)
{
	int ret;
	
	if(!(conn->flags & FLAG_SET_TAB)){
		ret = conn->server.SetTableName(conn->tab_name);
		if(ret != 0){
			snprintf(conn->errmsg, sizeof(conn->errmsg), "set table error: %s", conn->server.ErrorMessage());
			return ret;
		}
		conn->flags |= FLAG_SET_TAB;
	}
	
	if(!(conn->flags & FLAG_KEY_TYPE)){
		ret = conn->server.AddKey(conn->key_name, key_type_map[conn->key_type]);
		if(ret != 0){
			snprintf(conn->errmsg, sizeof(conn->errmsg), "add key error");
			return ret;
		}
		conn->flags |= FLAG_KEY_TYPE;
	}
	
	return 0;
}

int api_ttc_connect(void *&ptr, const char *host, const int port, int timeout, int mergeValue)
{
	int ret;
	char port_str[20];
	TTC_CONN* conn;
	
	ptr = NULL;
	conn = new TTC_CONN;
	if(conn == NULL){
		return -ENOMEM;
	}
	ptr = conn;
	
	conn->mergeValue = mergeValue;
	if(port != 0)
		snprintf(port_str, sizeof(port_str), "%d", port);
	else
		port_str[0] = 0;
	ret = conn->server.SetAddress(host, port_str);
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "set address error: %s", conn->server.ErrorMessage());
		delete conn;
		ptr = NULL;
		return ret;
	}
	
	conn->server.SetAccessKey("0000010184d9cfc2f395ce883a41d7ffc1bbcf4e");
	conn->server.SetTimeout(timeout);
	
	return 0;
}

int api_ttc_close(void *ptr)
{
	CAST(TTC_CONN, conn);
	conn->server.Close();
	
	return 0;
}

int api_ttc_release(void* ptr)
{
	CAST(TTC_CONN, conn);
	delete conn;
	ptr = NULL;
	
	return 0;
}

int api_ttc_settable(void* ptr, const char* tab_name)
{
	int ret;
	CAST(TTC_CONN, conn);
	
	ret = conn->server.SetTableName(tab_name);
	if(ret == 0){
		NAME(tab_name, tab_name);
		conn->flags |= FLAG_SET_TAB;
	}
	
	return ret;
}

int api_ttc_setkey(void* ptr, const char* key_name, int type)
{
	int ret;
	CAST(TTC_CONN, conn);
	
	if(type < 0 || type > TTC_KEY_TYPE_STRING){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "invalid key type: %d", type);
		return -EC_BAD_KEY_TYPE;
	}
	ret = conn->server.AddKey(key_name, key_type_map[conn->key_type]);
	if(ret == 0){
		NAME(key_name, key_name);
		conn->key_type = type;
		conn->flags |= FLAG_KEY_TYPE;
	}
	
	return ret;
}

int api_ttc_setval(void* ptr, const char* val_name)
{
	CAST(TTC_CONN, conn);
	
	NAME(val_name, val_name);
	
	return 0;
}

int api_ttc_setlen(void* ptr, const char* len_name)
{
	CAST(TTC_CONN, conn);
	
	NAME(len_name, len_name);
	
	return 0;
}

static int set_key(TTC::Request& req, TTC_CONN* conn, KV_PAIR *kv)
{
	int ret;
	
	switch(conn->key_type){
		case TTC_KEY_TYPE_INT:
			ret = req.AddKeyValue(conn->key_name, strtoll(kv->key, NULL, 0));
			if(ret != 0)
				snprintf(conn->errmsg, sizeof(conn->errmsg), "add int-key value error");
			break;
		
		case TTC_KEY_TYPE_STRING:
			ret = req.AddKeyValue(conn->key_name, kv->key);
			if(ret != 0)
				snprintf(conn->errmsg, sizeof(conn->errmsg), "add string-key value error");
			break;
		
		default:
			snprintf(conn->errmsg, sizeof(conn->errmsg), "invalid key type: %d", conn->key_type);
			ret = -EC_BAD_KEY_TYPE;
			break;
	}
	
	return ret;
}

int api_ttc_set(void *ptr, KV_PAIR *kv)
{
	int ret;
	CAST(TTC_CONN, conn);
	
	ret = connect_init(conn);
	if(ret != 0)
		return ret;
	
	TTC::ReplaceRequest req(&(conn->server));
	ret = set_key(req, conn, kv);
	if(ret != 0)
		return ret;
	
	char* real_val = NULL;
	if(conn->mergeValue){
		real_val = (char*)MALLOC(kv->vlen+2*4);
		if(real_val == NULL){
			snprintf(conn->errmsg, sizeof(conn->errmsg), "malloc error: %m");
			return -ENOMEM;
		}
		memcpy(real_val, &(kv->flags), 4);
		memcpy(real_val+4, &(kv->expire_time), 4);
		memcpy(real_val+8, kv->value, kv->vlen);
		ret = req.Set(conn->val_name, real_val, kv->vlen+8);
	}
	else{
		ret = req.Set(conn->len_name, kv->vlen);
		if(ret == 0)
			ret = req.Set(conn->val_name, kv->value, kv->vlen);
		if(ret == 0)
			ret = req.Set(conn->flag_name, kv->flags);
		if(ret == 0)
			ret = req.Set(conn->exp_name, kv->expire_time);
	}
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "set field error: %d", ret);
		FREE(real_val);
		return ret;
	}
	
	TTC::Result result; 
	ret = req.Execute(result);
	FREE(real_val);
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "execute set error: %d, %s", ret, result.ErrorMessage());
		return ret;
	} 
	
	return 0;
}

int api_ttc_delete(void *ptr, KV_PAIR kvs[], int count)
{
	int ret;
	CAST(TTC_CONN, conn);
	
	ret = connect_init(conn);
	if(ret != 0)
		return ret;
	
	TTC::DeleteRequest req(&(conn->server));
	for(int i=0; i<count; i++){
		ret = set_key(req, conn, kvs+i);
		if(ret != 0)
			return ret;
	}
	
	TTC::Result result; 
	ret = req.Execute(result);
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "execute delete error: %d, %s", ret, result.ErrorMessage());
		return ret;
	}
	
	return 0;
}

int api_ttc_get(void *ptr, KV_PAIR kvs[], int count)
{
	int ret;
	CAST(TTC_CONN, conn);
	
	ret = connect_init(conn);
	if(ret != 0)
		return ret;
	
	TTC::GetRequest req(&(conn->server));
	for(int i=0; i<count; i++){
		ret = set_key(req, conn, kvs+i);
		if(ret != 0)
			return ret;
		kvs[i].value = NULL;
		kvs[i].vlen = 0;
	}
	ret = req.Need(conn->key_name);
	if(ret == 0)
		ret = req.Need(conn->val_name);
	if(!(conn->mergeValue)){
		if(ret == 0)
			ret = req.Need(conn->flag_name);
		if(ret == 0)
			ret = req.Need(conn->exp_name);
		if(ret == 0)
			ret = req.Need(conn->len_name);
	}
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "set need error: %d", ret);
		return ret;
	}
	
	TTC::Result result; 
	ret = req.Execute(result);
	if(ret != 0){
		snprintf(conn->errmsg, sizeof(conn->errmsg), "execute get error: %d, %s", ret, result.ErrorMessage());
		return ret;
	}
	
	int idx = 0;
	int len;
	const char* val;
	for(int i=0; i<result.NumRows(); i++){
		ret = result.FetchRow();
		if(ret != 0){ 
			snprintf(conn->errmsg, sizeof(conn->errmsg), "fetch row error: %d, %s", ret, result.ErrorMessage());
			return ret;
		}
		
		for(int j=0; j<count; j++){
			idx = (idx+1)%count;
			if(strcasecmp(kvs[idx].key, result.StringValue(conn->key_name)) == 0){
				val = result.BinaryValue(conn->val_name, len);
				if(conn->mergeValue){
					if(len < 8){
						snprintf(conn->errmsg, sizeof(conn->errmsg), "key[%s]'s data len incorrect, it should large than 8", kvs[idx].key);
						return -EC_ERROR_BASE;
					}
					memcpy(&(kvs[idx].flags), val, 4);
					memcpy(&(kvs[idx].expire_time), val+4, 4);
					kvs[idx].vlen = len-8;
					kvs[idx].value = (char*)MALLOC(kvs[idx].vlen);
					if(kvs[idx].value == NULL){
						snprintf(conn->errmsg, sizeof(conn->errmsg), "malloc error: %m");
						return -ENOMEM;
					}
					memcpy(kvs[idx].value, val+8, kvs[idx].vlen);
				}
				else{
					kvs[idx].flags = result.IntValue(conn->flag_name);
					kvs[idx].expire_time = result.IntValue(conn->exp_name);
					kvs[idx].vlen = result.IntValue(conn->len_name);
					
					if(len != kvs[idx].vlen){
						snprintf(conn->errmsg, sizeof(conn->errmsg), "key[%s]'s data len incorrect, len[%d] real-len[%d]", kvs[idx].key, kvs[idx].vlen, len);
						return -EC_ERROR_BASE;
					}
					kvs[idx].value = (char*)MALLOC(len);
					if(kvs[idx].value == NULL){
						snprintf(conn->errmsg, sizeof(conn->errmsg), "malloc error: %m");
						return -ENOMEM;
					}
					memcpy(kvs[idx].value, val, len);
				}
				idx = (idx+1)%count;
				break;
			}
		}
	}
	
	return 0;
}


