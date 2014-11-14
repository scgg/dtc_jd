<?php
if(!defined("INT_MAX")) define("INT_MAX", 4294967296);

/**
 * 生成数据库表的增查删改操作,其中包括DAO、MOD、相关Htm页面等等
 * @author 付学宝
 * @version 1.0
 * @created 06-三月-2011 21:37:53
 */
class TPL2CODE
{

	/**
	 * 错误编码
	 */
	public static $errCode = 0;
	/**
	 * 错误信息,无错误为''
	 */
	public static $errMsg  = '';
	/**
	 * 清除错误信息,在每个函数的开始调用
	 */
	private static function clearError()
	{
		self::$errCode = 0;
		self::$errMsg	= '';
	}
	
	/**
	 * 生成TTC配置文件
	 *  
	 * @param tDtl    描述表的详细信息
	 */
	static function buildDTCfg($tDtl, $dir, $path)
	{
		$str = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_common/";
		$tpl = new Template($str); 
		$tpl->set_file("main", "dtc.conf");
		$tpl->set_var('DB_KEY',    $tDtl['DB_KEY']);
		$tpl->set_var('IP',    $tDtl['IP']);
		$tpl->set_var('PORT',    $tDtl['PORT']);
		$tpl->set_var('TOKEN',    $tDtl['TOKEN']);
		$tpl->set_var('TABLE_KEY', $tDtl['TABLE_KEY']);
		$tpl->set_var('TABLE_NAME',$tDtl['TABLE_NAME']);
		$tpl->set_var('MACHINE_NUM',  $tDtl['MACHINE_NUM']);
		$tpl->set_var('DB_IP',   $tDtl['DB_IP']);
		$tpl->set_var('DB_USER', $tDtl['DB_USER']);
		$tpl->set_var('DB_PASS', $tDtl['DB_PASS']);
		$tpl->set_var('DEPLOY',  $tDtl['DEPLOY']);
		$tpl->set_var('PRKEY_NUM', $tDtl['PRKEY_NUM']);
		$tpl->set_var('DB_MIN_NUM',$tDtl['DB_MIN_NUM']);
		$tpl->set_var('DB_MAX_NUM',$tDtl['DB_MAX_NUM']);
		$tpl->set_var('DB_INDEX',  $tDtl['DB_INDEX']);
		$tpl->set_var('PROCS_NUM', $tDtl['PROCS_NUM']);
		$tpl->set_var('WRITE_PROCS_NUM', $tDtl['WRITE_PROCS_NUM']);
		$tpl->set_var('COMMIT_PROCS_NUM',$tDtl['COMMIT_PROCS_NUM']);
		$tpl->set_var('TABLE_DIV_NUM', $tDtl['TABLE_DIV_NUM']);
		$tpl->set_var('TABLE_MOD_NUM', $tDtl['TABLE_MOD_NUM']);

		$fieldNum = count($tDtl['FIELDS']);
		$tpl->set_var('FIELD_NUM', $fieldNum);
		$tpl->set_block('main', 'FIELD', 'FIELDS');
		$itLst = $tDtl['FIELDS'];
		$i = 1;
		foreach ($itLst as $it){
			$tpl->set_var('FIELD_INDEX',$i);
			$tpl->set_var('FIELD_NAME', $it['name']);
			$tpl->set_var('FIELD_TYPE', $it['type']);
			$tpl->set_var('FIELD_SIZE', $it['size']);
			$tpl->parse('FIELDS', 'FIELD', true);
			$i++;
		}
		$tpl->parse("main", "main");
		$v = $tpl->get_var("main");
		return self::writeFile($dir . '/', "{$tDtl['TABLE_NAME']}.conf", $v);
	}
 	/**
	 *生成php配置代码	
	 */
	static function buildDTCPHP($tDtl, $dir, $path)
	{
		self::clearError();
		$cfgFile = $dir . '/' . $tDtl['TABLE_NAME'] . '.conf';
		$dtcCfg = parse_ini_file($cfgFile, true);
		if (!is_array($dtcCfg)){
			self::$errCode= 100;
			self::$errMsg = '读取TTC配置文件失败';
			return false;
		}
		if (!isset($dtcCfg['FIELD1']) || !isset($dtcCfg['TABLE_DEFINE']['TableName'])){
			self::$errCode= 100;
			self::$errMsg = 'TTC配置文件格式错误';
			return false;
		}
		$dtcFieldLst = array();
		for ($i = 1; ; ++$i)
		{
			$key = 'FIELD'.$i;
			if (!key_exists($key, $dtcCfg)) break;
			if (!is_array($dtcCfg[$key])) break;
			$index = $i - 1;
			$dtcFieldLst[$index] = array();
			$dtcFieldLst[$index]['name'] = $dtcCfg[$key]['FieldName'];
			if (isset($dtcCfg[$key]['DefaultValue']) && $dtcCfg[$key]['DefaultValue']=="auto_increment" ){ 
				$dtcFieldLst[$index]['auto'] = true; 
			}
			if ($dtcCfg[$key]['FieldType'] == 1){   //signed
				$dtcFieldLst[$index]['type'] = 1;		
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:
						$dtcFieldLst[$index]['min'] = -128;
						$dtcFieldLst[$index]['max'] = 127;
						break;
					case 2:
						$dtcFieldLst[$index]['min'] = -65536;
						$dtcFieldLst[$index]['max'] = 0x7FFF;
						break;
					case 4:
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					case 8:   //当做字符串处理
//						$dtcFieldLst[$index]['min'] = -9223372036854775808;
//						$dtcFieldLst[$index]['max'] = 0x7FFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 2){   //unsigned
				$dtcFieldLst[$index]['type'] = 1;		
				$dtcFieldLst[$index]['min'] = 0;
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:				
						$dtcFieldLst[$index]['max'] = 255;
						break;
					case 2:
						$dtcFieldLst[$index]['max'] = 0xFFFF;
						break;
					case 4:
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					case 8:   //当做字符串处理
//						$dtcFieldLst[$index]['min'] = 0;
//						$dtcFieldLst[$index]['max'] = 0xFFFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = 0;
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 4){   //STRING
				$dtcFieldLst[$index]['type']= 2;		
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 5){   //STRING
				$dtcFieldLst[$index]['type']= 3;		
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 3){   //FLOAT
				$dtcFieldLst[$index]['type']= 4;		
				$dtcFieldLst[$index]['min'] = -0xFFFFFFFF;
				$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
			}else{
				self::$errCode= 100;
				self::$errMsg = "FILED[" . $dtcCfg[$key]['FieldName'] . "TYPE IS INVALID ";
				return false;
			}
		}
		$itLst = $tDtl['FIELDS'];
		$str = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_php/";
		$tpl = new Template($str); 
		$tpl->set_file("main", "dao_tc.php");
		$tpl->set_var('TTCKEY', $tDtl['TTCKEY']);
		$tpl->set_var('IP',    $tDtl['IP']);
		$tpl->set_var('PORT',    $tDtl['PORT']);
		$tpl->set_var('TOKEN',    $tDtl['TOKEN']);
		$tpl->set_var('PRIKEY', $tDtl['FIELDS'][0]['name']);
		$tpl->set_var('TABLE_NAME',   $tDtl['TABLE_NAME']);

		$tpl->set_block('main', 'FIELD1', 'FIELD1S');
		$tpl->set_block('main', 'FIELD2', 'FIELD2S');
		
		$len  = self::getFieldsMaxLen($itLst);
		$len += 2;
		foreach ($dtcFieldLst as $it){
			$strName = str_pad("'".$it['name']."'", $len);
			$tpl->set_var('FIELD1_NAME', $strName);		
			$tpl->set_var('FIELD1_TYPE', $it['type']);
			$tpl->set_var('MIN_NUM', $it['min']);
			$tpl->set_var('MAX_NUM', $it['max']);
			$tpl->parse('FIELD1S', 'FIELD1', true);
		}
		foreach ($itLst as $it){
			$strName = str_pad("'".$it['name']."'", $len);
			$tpl->set_var('FIELD2_NAME', $strName);
			$tmp = ($it['type']!=4 && $it['type']!=5) ? ' XXX' : "'XXX'";
			$tpl->set_var('FIELD2_VALUE',  $tmp);
			$tpl->set_var('FIELD2_NAME_CH',$it['cname']);
			$tpl->parse('FIELD2S', 'FIELD2', true);
		}
		$tpl->parse("main","main");
		$v = $tpl->get_var("main");
		self::writeFile($dir . '/' ,"I{$tDtl['TABLE_NAME']}DTC.php", $v);
		
		$errorCode=false;
		$ss = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_php/DTC.php ";
		$cmd = 'cp -p ' . $ss . $dir;
		exec($cmd, $errorCode);
		return $errorCode;
	}
	/*
	 *生成java配置代码
	 *
	 */
	static function buildDTCJAVA($tDtl, $dir, $path)
	{
		self::clearError();
		$cfgFile = $dir . '/' . $tDtl['TABLE_NAME'] . '.conf';
		$dtcCfg = parse_ini_file($cfgFile, true);
		if (!is_array($dtcCfg)){
			self::$errCode= 100;
			self::$errMsg = '读取TTC配置文件失败';
			return false;
		}
		if (!isset($dtcCfg['FIELD1']) || !isset($dtcCfg['TABLE_DEFINE']['TableName'])){
			self::$errCode= 100;
			self::$errMsg = 'TTC配置文件格式错误';
			return false;
		}
		$dtcFieldLst = array();
		for ($i = 1; ; ++$i)
		{
			$key = 'FIELD'.$i;
			if (!key_exists($key, $dtcCfg)) break;
			if (!is_array($dtcCfg[$key])) break;
			$index = $i - 1;
			$dtcFieldLst[$index] = array();
			$dtcFieldLst[$index]['name'] = $dtcCfg[$key]['FieldName'];
			if (isset($dtcCfg[$key]['DefaultValue']) && $dtcCfg[$key]['DefaultValue']=="auto_increment" ){ 
				$dtcFieldLst[$index]['auto'] = true; 
			}
			if ($dtcCfg[$key]['FieldType'] == 1){   //signed
				$dtcFieldLst[$index]['type'] = 1;		
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:
						$dtcFieldLst[$index]['min'] = -128;
						$dtcFieldLst[$index]['max'] = 127;
						break;
					case 2:
						$dtcFieldLst[$index]['min'] = -65536;
						$dtcFieldLst[$index]['max'] = 0x7FFF;
						break;
					case 4:
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					case 8:   //当做字符串处理
//						$dtcFieldLst[$index]['min'] = -9223372036854775808;
//						$dtcFieldLst[$index]['max'] = 0x7FFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 2){   //unsigned
				$dtcFieldLst[$index]['type'] = 2;		
				$dtcFieldLst[$index]['min'] = 0;
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:				
						$dtcFieldLst[$index]['max'] = 255;
						break;
					case 2:
						$dtcFieldLst[$index]['max'] = 0xFFFF;
						break;
					case 4:
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					case 8:   //当做字符串处理
//						$dtcFieldLst[$index]['min'] = 0;
//						$dtcFieldLst[$index]['max'] = 0xFFFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = 0;
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 4){   //STRING
				$dtcFieldLst[$index]['type']= 4;		
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 5){   //STRING
				$dtcFieldLst[$index]['type']= 5;		
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 3){   //FLOAT
				$dtcFieldLst[$index]['type']= 3;		
				$dtcFieldLst[$index]['min'] = -0xFFFFFFFF;
				$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
			}else{
				self::$errCode= 100;
				self::$errMsg = "FILED[" . $dtcCfg[$key]['FieldName'] . "TYPE IS INVALID ";
				return false;
			}
		}
		$itLst = $tDtl['FIELDS'];
		$str = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_java/";
		$tpl = new Template($str); 
		$tpl->set_file("main", "DTC.java");
		$tpl->set_var('Dtckey', $tDtl['TTCKEY']);
		$tpl->set_var('IP',    $tDtl['IP']);
		$tpl->set_var('Port',    $tDtl['PORT']);
		$tpl->set_var('Token',    $tDtl['TOKEN']);
		//$tpl->set_var('PRIKEY', $tDtl['FIELDS'][0]['name']);
		$tpl->set_var('TableName',   $tDtl['TABLE_NAME']);

		$tpl->set_block('main', 'FIELD1', 'FIELD1S');
		//$tpl->set_block('main', 'FIELD2', 'FIELD2S');
		
		//$len  = self::getFieldsMaxLen($itLst);
		foreach ($dtcFieldLst as $it){
			$strName = str_pad($it['name'], strlen($it['name']));
			$tpl->set_var('FIELD1Name', $strName);		
			$tpl->set_var('FIELD1Type', $it['type']);
			$tpl->set_var('minsize', $it['min']);
			$tpl->set_var('maxsize', $it['max']);
			$tpl->parse('FIELD1S', 'FIELD1', true);
		}
		foreach ($itLst as $it){
			$strName = str_pad($it['name'], strlen($it['name']));
			$tpl->set_var('FIELD2Name', $strName);
			$tmp = ($it['type']!=4 && $it['type']!=5) ? ' XXX' : "'XXX'";
			$tpl->set_var('FIELD2_VALUE',  $tmp);
			$tpl->set_var('FIELD2_NAME_CH',$it['cname']);
			$tpl->parse('FIELD2S', 'FIELD2', true);
		}

		$tpl->parse("main","main");
		$v = $tpl->get_var("main");
		self::writeFile($dir . '/', "DTC{$tDtl['TABLE_NAME']}.java", $v);

		$tpl1 = new Template($str);
                $tpl1->set_file("main1", "IDTC.java");
		$tpl1->set_var('TableName',   $tDtl['TABLE_NAME']);
		$tpl1->parse("main1","main1");
                $v1 = $tpl1->get_var("main1");		
		self::writeFile($dir .'/', "I{$tDtl['TABLE_NAME']}DTC.java", $v1);
		
		$ss = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_java/Field.java ";
		$cmd = 'cp -p ' . $ss . $dir;
		$errorCode="";
		exec($cmd, $errorCode);
		return $errorCode;
	}	
	
	/*
	 *生成c++配置代码
	*
	*/
	static function buildDTCCplus($tDtl, $dir, $path)
	{
		self::clearError();
		$cfgFile = $dir . '/' . $tDtl['TABLE_NAME'] . '.conf';
		$dtcCfg = parse_ini_file($cfgFile, true);
		if (!is_array($dtcCfg)){
			self::$errCode= 100;
			self::$errMsg = '读取TTC配置文件失败';
			return false;
		}
		if (!isset($dtcCfg['FIELD1']) || !isset($dtcCfg['TABLE_DEFINE']['TableName'])){
			self::$errCode= 100;
			self::$errMsg = 'TTC配置文件格式错误';
			return false;
		}
		$dtcFieldLst = array();
		for ($i = 1; ; ++$i)
		{
			$key = 'FIELD'.$i;
			if (!key_exists($key, $dtcCfg)) break;
			if (!is_array($dtcCfg[$key])) break;
			$index = $i - 1;
			$dtcFieldLst[$index] = array();
			$dtcFieldLst[$index]['name'] = $dtcCfg[$key]['FieldName'];
			if (isset($dtcCfg[$key]['DefaultValue']) && $dtcCfg[$key]['DefaultValue']=="auto_increment" ){
				$dtcFieldLst[$index]['auto'] = true;
			}
			if ($dtcCfg[$key]['FieldType'] == 1){   //signed
				$dtcFieldLst[$index]['type'] = 1;
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:
						$dtcFieldLst[$index]['min'] = -128;
						$dtcFieldLst[$index]['max'] = 127;
						break;
					case 2:
						$dtcFieldLst[$index]['min'] = -65536;
						$dtcFieldLst[$index]['max'] = 0x7FFF;
						break;
					case 4:
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					case 8:   //当做字符串处理
						//						$dtcFieldLst[$index]['min'] = -9223372036854775808;
						//						$dtcFieldLst[$index]['max'] = 0x7FFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = -2147483648;
						$dtcFieldLst[$index]['max'] = 0x7FFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 2){   //unsigned
				$dtcFieldLst[$index]['type'] = 2;
				$dtcFieldLst[$index]['min'] = 0;
				switch ($dtcCfg[$key]['FieldSize']){
					case 1:
						$dtcFieldLst[$index]['max'] = 255;
						break;
					case 2:
						$dtcFieldLst[$index]['max'] = 0xFFFF;
						break;
					case 4:
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					case 8:   //当做字符串处理
						//						$dtcFieldLst[$index]['min'] = 0;
						//						$dtcFieldLst[$index]['max'] = 0xFFFFFFFFFFFFFFFF;
						$dtcFieldLst[$index]['min'] = 0;
						$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
						break;
					default:
						self::$errCode= 100;
						self::$errMsg = "FILED[".$dtcCfg[$key]['FieldName']."] SIZE TOO BIG ".$dtcCfg[$key]['FieldSize'];
						return false;
				}
			}else if($dtcCfg[$key]['FieldType'] == 4){   //STRING
				$dtcFieldLst[$index]['type']= 4;
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 5){   //STRING
				$dtcFieldLst[$index]['type']= 5;
				$dtcFieldLst[$index]['min'] = 0;
				$dtcFieldLst[$index]['max'] = $dtcCfg[$key]['FieldSize'];
			}else if($dtcCfg[$key]['FieldType'] == 3){   //FLOAT
				$dtcFieldLst[$index]['type']= 3;
				$dtcFieldLst[$index]['min'] = -0xFFFFFFFF;
				$dtcFieldLst[$index]['max'] = 0xFFFFFFFF;
			}else{
				self::$errCode= 100;
				self::$errMsg = "FILED[" . $dtcCfg[$key]['FieldName'] . "TYPE IS INVALID ";
				return false;
			}
		}
		$itLst = $tDtl['FIELDS'];
		$str = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_c++/";
		$tpl = new Template($str);
		$tpl->set_file("main", "dtc.h");
		$tpl->set_var('Dtckey', $tDtl['TTCKEY']);
		$tpl->set_var('IP',    $tDtl['IP']);
		$tpl->set_var('Port',    $tDtl['PORT']);
		$tpl->set_var('Token',    $tDtl['TOKEN']);
		//$tpl->set_var('PRIKEY', $tDtl['FIELDS'][0]['name']);
		$tpl->set_var('TableName',   $tDtl['TABLE_NAME']);
	
		$tpl->set_block('main', 'FIELD1', 'FIELD1S');
		//$tpl->set_block('main', 'FIELD2', 'FIELD2S');
	
		//$len  = self::getFieldsMaxLen($itLst);
		foreach ($dtcFieldLst as $it){
			$strName = str_pad($it['name'], strlen($it['name']));
			$tpl->set_var('FIELD1Name', $strName);
			$tpl->set_var('FIELD1Type', $it['type']);
			$tpl->set_var('minsize', $it['min']);
			$tpl->set_var('maxsize', $it['max']);
			$tpl->parse('FIELD1S', 'FIELD1', true);
		}
		foreach ($itLst as $it){
			$strName = str_pad($it['name'], strlen($it['name']));
			$tpl->set_var('FIELD2Name', $strName);
			$tmp = ($it['type']!=4 && $it['type']!=5) ? ' XXX' : "'XXX'";
			$tpl->set_var('FIELD2_VALUE',  $tmp);
			$tpl->set_var('FIELD2_NAME_CH',$it['cname']);
			$tpl->parse('FIELD2S', 'FIELD2', true);
		}
	
		$tpl->parse("main","main");
		$v = $tpl->get_var("main");
		self::writeFile($dir . '/', "dtc_{$tDtl['TABLE_NAME']}.h", $v);
	
		$tpl1 = new Template($str);
		$tpl1->set_file("main1", "dtc.cpp");
		$tpl1->set_var('TableName',   $tDtl['TABLE_NAME']);
		$tpl1->parse("main1","main1");
		$v1 = $tpl1->get_var("main1");
		self::writeFile($dir . '/', "dtc_{$tDtl['TABLE_NAME']}.cpp", $v1);
		
		$errorCode=false;
		$ss = "/export/servers/" . $path . "/PhpCodeFactory/tpl/tpl_c++/ttcapi.h ";
		$cmd = 'cp -p ' . $ss . $dir;
		exec($cmd, $errorCode);
		
		return $errorCode;
	}
	
	
	//截取模版中指定变量的最后几位字符
	//pos:1 截取后面字符,2 截取前面字符
	static function cutTplVarVal(&$tpl, $name, $n, $pos=1){
		$v = $tpl->get_var($name);
		if(empty($v)) return;
		$st = ($pos == 1) ? 0 : $n;
		$v = substr($v, $st, strlen($v)-$n);
		$tpl->set_var($name, $v);
	}
	
	static function writeFile($dir, $file, $content){
		if(!file_exists($dir)) mkdir($dir);
		$fullFile = $dir.$file;
		//文件存在不能覆盖
//		if(file_exists($fullFile)) return false;
		return file_put_contents($fullFile, $content);
	}

	/**
	 * 获得字段名的最大长度
	 * 
	 * @param fields    字段名配置信息
	 */
	static function getFieldsMaxLen($fields, $key='name')
	{
		$maxLen = 0;
		foreach ($fields as $v){
			$tmp = strlen($v[$key]);
			if($tmp > $maxLen){
				$maxLen = $tmp;
			}
		}
		return $maxLen;
	}
	

}

//End Of Script
