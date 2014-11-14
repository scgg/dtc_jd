<?php
$ttc_srv  =&  new tphp_ttc_server();//只要server不析构，后台会保持长连接

$ttc_srv->int_key();

$ttc_srv->set_tablename("t_dtc_example");

$ttc_srv->set_address("192.168.214.62", "10009");

$ttc_srv->set_accesskey("0000010284d9cfc2f395ce883a41d7ffc1bbcf4e");

$ttc_srv->set_timeout(5);

$ttc_request = new tphp_ttc_request($ttc_srv, 4);

$ttc_request->set_key(100008);

$ttc_request->need("name");

$ttc_request->need("age");

$ttc_result = new tphp_ttc_result();

$ttc_request->execute($ttc_result);

printf("num:%d\n",$ttc_result->affected_rows());

printf("fetch_row retcode:%d\n", $ttc_result->fetch_row());

printf("set success, result code: %d\n", $ttc_result->result_code ());

printf("name:%s\n", $ttc_result->string_value("name"));

printf("age:%d\n", $ttc_result->int_value("age"));

?>
