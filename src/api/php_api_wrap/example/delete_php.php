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

//构建 delete 请求，即 对应下面填的参数 8 ，插入数据
$ttc_delete = new tphp_ttc_request($ttc_srv, 8); 
$ret = $ttc_delete->set_key($key);
printf("delete set key.ret:%d\n", $ret);
$ttc_result = new tphp_ttc_result();
$ret = $ttc_delete->execute($ttc_result);
printf("delete execute end.ret:%d\n", $ret);
if($ret == 0)
{	
	printf("delect success!\n");
}
?>
