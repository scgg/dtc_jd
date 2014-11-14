<?php
echo $_SERVER['PHP_SELF'];
$array = explode("/",$_SERVER['PHP_SELF']);
$v_path=$array[3];

require_once('/export/servers/' . $v_path . '/PhpCodeFactory/lib/Template.php');
require_once('/export/servers/' . $v_path . '/PhpCodeFactory/tpl/tpl_common/TPL2CODE.php');


//MACHINE_NUM:一共有多少台Db server。V2.0.3开始，不分库的话，可以不用设置这个值
//Deploy:分表分库的类型： 不分库分表=0, 只分库=1,  只分表=2,  既分库也分表=3
//#None=0, Signed=1, Unsigned=2, Float=3, String=4, Binary=5

function show_usage()
{	
	printf("usage: php codeGenerator.php [--config=]  [--java_dir=] [--cplus_dir] [--php_dir] \n");
	printf("	-h             help for using tt.php\n");
	printf("    --config       Specify config file\n");
	printf("    --agent_ip     agent ip\n");
	printf("    --agent_port   agent port\n");
	printf("    --access_token access token\n");
	printf("	--java_dir     Final ouput java code path\n");
	printf("	--cplus_dir    Final output c++ code path\n");
	printf("	--php_dir      Final output php code path\n");
	printf("\n");
	printf("\n");
	
}

$shortopts  = "";
$shortopts .= "h"; // These options do not accept values

$longopts  = array(
		"config::",       // Optional value
		"agent_ip::",     // Optional value
		"agent_port::",   // Optional value
		"access_token::", // Optional value
		"java_dir::",     // Optional value
		"cplus_dir::",    // Optional value
		"php_dir::",      // Optional value
);

$options = getopt($shortopts, $longopts);
if(empty($options)){
	show_usage();
	exit(1);
}
$cfg = null;
$agentip = null;
$agentport = null;
$accesstoken = null;
$java_dir = null;
$php_dir = null;
$cplus_dir = null;

foreach($options as $k =>$v){
	if($k == 'h'){
		show_usage();
		exit(0);
	}
	else if($k == 'config'){
		$cfg = $v;
	}
	else if($k == 'agent_ip'){
		$agentip = $v;
	}
	else if($k == 'agent_port'){
		$agentport = $v;
	}
	else if($k == 'access_token'){
		$accesstoken = $v;
	}
	else if($k == 'java_dir'){
		$java_dir = $v;
	}
	else if($k == 'cplus_dir'){
		$cplus_dir = $v;
	}
	else if($k == 'php_dir'){
		$php_dir = $v;
	}
	else{
		show_usage();
		exit(0);
	}
}

//require_once($config_dir . '/config.php');
require_once('/export/servers/' . $v_path . '/PhpCodeFactory/code/parseConfig.php');
$tDtl = array();
$tDtl['IP'] = $agentip;
$tDtl['PORT'] = $agentport;
$tDtl['TOKEN'] = $accesstoken;
parseConfig($cfg, $tDtl);

if($php_dir != null){
	TPL2CODE::buildDTCfg($tDtl, $php_dir, $v_path);
	TPL2CODE::buildDTCPHP($tDtl, $php_dir, $v_path);
	var_dump(TPL2CODE::$errMsg);
}

if($java_dir != null){
	TPL2CODE::buildDTCfg($tDtl, $java_dir, $v_path);
	TPL2CODE::buildDTCJAVA($tDtl, $java_dir, $v_path);
	var_dump(TPL2CODE::$errMsg);
}

if($cplus_dir != null){
	TPL2CODE::buildDTCfg($tDtl, $cplus_dir, $v_path);
	TPL2CODE::buildDTCCplus($tDtl, $cplus_dir, $v_path);
	var_dump(TPL2CODE::$errMsg);
}
//End Of Script

