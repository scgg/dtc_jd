<?php
$tDtl = array('IP'=>'192.168.214.62','PORT'=>'10901', 'TOKEN'=>'0000090184d9cfc2f395ce883a41d7ffc1bbcf4e','DB_KEY' => 'test','TABLE_KEY' => 't_dtc_example','TABLE_NAME' => 't_dtc_example',
		'MACHINE_NUM' => '1','DB_IP' => '192.168.214.62:3306','DB_USER' => 'root','DB_PASS' => 'DTC@ecc2014','DEPLOY' => '0','DB_MIN_NUM' => '0','DB_MAX_NUM' => '0','DB_INDEX' => '0-0','PRKEY_NUM' => '1',
		'PROCS_NUM' => '10','WRITE_PROCS_NUM' => '10','COMMIT_PROCS_NUM' => '10','TABLE_DIV_NUM' => '1','TABLE_MOD_NUM' => '10'
);

$tDtl['FIELDS'] = array(
		array("name"=>'uid'   , "cname"=>'uid', "auto"=>0, "primary"=>0, "type"=>2, "size"=>4),
		array("name"=>'name'  , "cname"=>'name', "auto"=>0, "primary"=>0, "type"=>4, "size"=>100),
		array("name"=>'city'  , "cname"=>'city', "auto"=>0, "primary"=>0, "type"=>4, "size"=>100),
		array("name"=>'descr'  , "cname"=>'descr', "auto"=>0, "primary"=>0, "type"=>5, "size"=>255),
		array("name"=>'age' , "cname"=>'age', "auto"=>0, "primary"=>0, "type"=>2, "size"=>4),
		array("name"=>'salary' , "cname"=>'salary', "auto"=>0, "primary"=>0, "type"=>2, "size"=>4),
);
$ttckey = "uid";