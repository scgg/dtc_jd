<?php
//创建 dtc_api server对象
$ttc_srv  =&  new tphp_ttc_server();//只要server不析构，后台会保持长连接
//设置主键为int类型的
$ttc_srv->int_key();
//设置表名
$ttc_srv->set_tablename("t_dtc_example");
//设置ip/port
$ttc_srv->set_address("192.168.214.62", "10009");
//设置访问码
$ttc_srv->set_accesskey("0000010284d9cfc2f395ce883a41d7ffc1bbcf4e");
//设置单次网络IO超时
$ttc_srv->set_timeout(5);

$key = 100009;

//构建Insert 请求，插入数据
$ttc_insert = new tphp_ttc_request($ttc_srv, 6); 
$ret = $ttc_insert->set_key($key);
printf("insert set key.ret:%d\n", $ret);
//set函数是用来指定insert sql 语句中的set 字段名=value 的这样一个操作
$ttc_insert->set("uid", $key);
$ttc_insert->set_str("name", "tttt");
$ttc_insert->set_str("city", "sadsa");
$ttc_insert->set_str("descr", "hkjhsakjfhkjdsah");
$ttc_insert->set("age", 30);

$ttc_result = new tphp_ttc_result();
$ret = $ttc_insert->execute($ttc_result);
printf("insert execute end.ret:%d\n", $ret);
if($ret == 0)
{
	//清空result对象
	$ttc_result->reset();

	//创建get 请求request对象
	$ttc_get = new tphp_ttc_request($ttc_srv, 4);

	$ttc_get->set_key($key);
	//指定需要查询的字段
	$ttc_get->need("name");
	$ttc_get->need("city");
	$ttc_get->need("age");
	
	$ret = $ttc_get->execute($ttc_result);
	printf("get execute end.ret:%d\n", $ret);
	$ret = $ttc_result->fetch_row();
	printf("get fetch_row ret:%d\n", $ret);
	printf("uid:%d\n", $ttc_result->int_value("uid"));
	printf("name:%s\n", $ttc_result->string_value("name"));
	printf("city:%s\n", $ttc_result->string_value("city"));		
	printf("age:%d\n", $ttc_result->int_value("age"));
}
?>
