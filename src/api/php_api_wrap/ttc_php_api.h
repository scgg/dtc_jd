#ifndef __TTC_PHP_API_H__
#define __TTC_PHP_API_H__

const int TTC_KEY_TYPE_INT = 0;
const int TTC_KEY_TYPE_STRING = 1;

typedef struct _kv_pair
{
	char *key;
	char *value;
	int vlen;
	int flags;
	int expire_time;
} KV_PAIR;

/* 创建一个连接对象，对象指针保存在ptr。其他的接口必须传递这个ptr作为参数 */
extern int api_ttc_connect(void *&ptr, const char *host, const int port, int timeout, int mergeValue=1);

/* 关闭跟TTC的连接（但不释放连接对象，ptr仍然可以重用） */
extern int api_ttc_close(void *ptr);

/* 关闭连接并释放对象（ptr自动赋值为NULL） */
extern int api_ttc_release(void* ptr);

/* 除api_ttc_connect以外，其他接口返回错误时，用这个接口获取详细错误信息 */
extern const char* api_ttc_strerror(void* ptr);

/* 设置key-value（如果key不存在insert，否则update） */
extern int api_ttc_set(void *ptr, KV_PAIR *kv);

/* 批量删除key对应的数据（最多32个key） */
extern int api_ttc_delete(void *ptr, KV_PAIR kvs[], int count = 1);

/* 批量查询key对应的数据（最多32个key）。如果某个key对应的数据不存在，则value=NULL，vlen=0 */
extern int api_ttc_get(void *ptr, KV_PAIR kvs[], int count = 1);


//===================================================================================

/* 可选接口：设置ttc的表名（不调用接口则用默认表名t_data） */
extern int api_ttc_settable(void* ptr, const char* tab_name);

/* 可选接口：设置ttc的key（不调用接口则用默认key名称key，类型为字符串） */
extern int api_ttc_setkey(void* ptr, const char* key_name, int type);

/* 可选接口：设置ttc的value字段名称（不调用接口则用默认字段名data） */
extern int api_ttc_setval(void* ptr, const char* val_name);

/* 可选接口：设置ttc的len字段名称（不调用接口则用默认字段名len） */
extern int api_ttc_setlen(void* ptr, const char* len_name);

#endif
