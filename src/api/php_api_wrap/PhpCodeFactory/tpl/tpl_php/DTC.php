<?php
if ( !defined('TPHP_DTC_OP_GET') ) {
	define( "TPHP_DTC_OP_GET", 4 );
}

if ( !defined('TPHP_DTC_OP_SELECT') ) {
	define( "TPHP_DTC_OP_SELECT", 4 );
}

define( "TPHP_DTC_OP_PURGE",    5 );
define( "TPHP_DTC_OP_INSERT",   6 );
define( "TPHP_DTC_OP_UPDATE",   7 );
define( "TPHP_DTC_OP_DELETE",   8 );
define( "TPHP_DTC_OP_REPLACE",  9 );
define( "TPHP_DTC_OP_FLUSH",    13 );



/**
 * dtc Manager, substitute old Config
 */

class DTCManager
{
	/**
	 * 保存项目中DTC的配置
	 *
	 * @var array
	 */
	private static $DTCCfg;
	
	private static $Cfg;
	
	/**
	 * 保存项目中DTC的句柄
	 *
	 * @var array
	 */
	private static $DTCResMap = array();
	
	/**
	 * 错误编码
	 *
	 * @var int
	 */
	public static $errCode = 0;
	
	/**
	 * 错误信息
	 *
	 * @var string
	 */
	public static $errMsg = '';
	
	
	/**
	 * 初始化配置变量
	 */
	private static function init()
	{
		global  $_DTC_CFG;
		// DTC 配置
		if (empty(self::$DTCCfg)) {
			if(isset($_DTC_CFG)){
				self::$DTCCfg = &$_DTC_CFG;
			} else {
				self::$DTCCfg = '';
			}
		}
	
	}
	
	/**
	 * 清除错误标识，在每个函数调用前调用
	 */
	private static function clearERR()
	{
		self::$errCode = 0;
		self::$errMsg  = '';
	}
	
	/**
	 * 获得 DTC 句柄
	 *
	 * @param	string	$key	资源的key
	 * @return 	DTC	返回DTC句柄
	 */
	public static function getDTC($key)
	{
		self::clearERR();
		self::init();
		// 如果在前面已创建该 dtc 资源，则直接返回
		if (isset(self::$DTCResMap[$key])){
			return self::$DTCResMap[$key];
		}
		// 判断参数
		if (!isset(self::$DTCCfg[$key])){
			self::$errCode = 20000;
			self::$errMsg = "no DTC config info for key {$key}";
			return false;
		}
		// cache 配置
		$cfg = self::$DTCCfg[$key];
		$dtc = new DTC($cfg);
		
		// 保存到类属性中
		self::$DTCResMap[$key] = $dtc;
		return 	self::$DTCResMap[$key];
	}
	
}

/**
 * 封装DTC的一些API
 * @author (DTC Team)
 * @date 20140820
 */
class DTC
{
	const INT = 1;
	const STRING = 2;
	const BINARY = 3;
	const FLOAT = 4;

	/**
	 * 配置标识
	 * 用于上报等操作
	 * @var string
	 */
	public $configId;

	/**
	 * 错误码
	 * @var int
	 */
	public $errCode;
	/**
	 * 错误信息
	 * @var string
	 */
	public $errMsg;
	/**
	 * 错误来源
	 * @var string
	 */
	public $errFrom;

	/**
	 * DTC返回码
	 * @var int
	 */
	public $dtcCode;

	/**
	 * dtc的配置，可以通过这个配置验证其数据的合法性等等
	 * @var array
	 */
	private $config;

	private $key_type;

	/**
	 * dtc数据连接
	 * @var tphp_ttc_server
	 */
	private $server = false;
	private $dtcKey = false;
	private $result = null;

	/**
	 * dtc key 通过这个key来获取其配置信息
	 *
	 * @param string dtcKey
	 */
	function __construct($config)
	{
		$this->dtcKey = $config['DTCKEY'];
		$this->config = $config;
	}

	function __destruct()
	{
		if ($this->server != false) {
			$this->server->close();
		}
	}

	/**
	 * 初始化dtc的配置,这个只有在真正操作的时候才会去做
	 *
	 * @return boolean
	 */
	private function init()
	{
		$this->clearERR();

		if (!extension_loaded('tphp_dtc') ) {
			if (!ini_get("safe_mode") && ini_get("enable_dl") ) {
				if ( !dl("tphp_dtc.so") ) {
					$this->errCode = 20000;
					$this->errMsg  = 'can not load tphp_dtc extension module';

					return false;
				}
			} else {
				$this->errCode = 20000;
				$this->errMsg  = 'can not load tphp_dtc extension module';

				return false;
			}
		}

		if ($this->server == false) {
			$this->server = new tphp_ttc_server();
			$errCode = $this->server->set_timeout($this->config['TimeOut']);
			if ($errCode != 0) {
				$this->errCode = $errCode;
				$this->errMsg  = $this->server->error_message();
				$this->server->close();

				return false;
			}

			$errCode = $this->server->set_tablename($this->config['TABLE']);
			if ($errCode != 0) {
				$this->errCode = $errCode;
				$this->errMsg  = $this->server->error_message();
				$this->server->close();

				return false;
			}

			$errCode = $this->server->set_address($this->config['IP'], $this->config['PORT']);
			if ($errCode != 0) {
				$this->errCode = $errCode;
				$this->errMsg  = $this->server->error_message();
				$this->server->close();

				return false;
			}

			$errCode = $this->server->set_accesskey($this->config['TOKEN']);
			if ($errCode != 0) {
				$this->errCode = $errCode;
				$this->errMsg  = $this->server->error_message();
				$this->server->close();

				return false;
			}
			
			// 此处加入对字符串key的支持
			$this->key_type = $this->config['FIELDS'][$this->config['KEY']]['type'];
			if ( $this->key_type == 1 ) {
				$errCode = $this->server->int_key();
			}
			elseif ( $this->key_type == 2 ) {
				$errCode = $this->server->string_key();
			}
			else  {
				$this->errCode = 20005;
				$this->errMsg  = 'invalid key type';
				$this->server->close();

				return false;
			}

			if ($errCode != 0) {
				$this->errCode = $errCode;
				$this->errMsg  = $this->server->error_message();
				$this->server->close();

				return false;
			}
		}

		return true;
	}

	/**
	 * 清理缓存
	 *
	 * @param mix $key 第一个key
	 * @param array $multikey 其它过滤条件
	 *
	 * @return boolean
	 */
	public function purge($key, array $multikey = array())
	{
		$this->clearERR();

		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}


		if ($this->checkKey($key) == false) {
			return false;
		}

		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_PURGE);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($key);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($key);
		} else {
			$this->errCode = 20018;
			$this->errMsg  = 'request failed';

			return false;
		}

		if($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} purge:{$key}: set_key error";

			return false;
		}
		/*当前版本的dtc暂不支持多key的需求
		if ( !empty($multikey) && is_array($multikey) ) {
			// 处理多字段key情况
			foreach ( $multikey as $mk => $mv ) {
				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}
				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20093;
					$this->errMsg  = "dtc config:{$this->dtcKey} purge:{$key}: set multikey error";

					return false;
				}
			}
		}*/

		$result = new tphp_ttc_result();
		$ret = $req->execute($result);
		$this->result = $result;

		//TODO ...
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = $errCode;
			//$this->errMsg  = "dtc config:{$this->dtcKey} purge:{$key} error,error code:{$errCode}";
			$this->errMsg = $result->error_message();
			$this->errFrom = $result->error_from();		
			return false;
		}

		return true;
	}

	/**
	 * 删除一条数据库记录
	 *
	 * @param key    数据库的主键
	 */
	public function delete($key, $multikey = array())
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		if ($this->checkKey($key) == false) {
			return false;
		}

		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_DELETE);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($key);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($key);
		} else {
			$this->errCode = 20019;
			$this->errMsg  = 'request failed';

			return false;
		}

		if($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} delete:{$key}: set_key error";

			return false;
		}
		/*当前版本的dtc暂不支持多key的需求
		if ( !empty($multikey) && is_array($multikey) ) {
			// 处理多字段key情况
			foreach ( $multikey as $mk => $mv ) {
				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}
				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20013;
					$this->errMsg  = "dtc config:{$this->dtcKey} delete:{$key}: set multikey error";

					return false;
				}
			}
		}*/

		$result = new tphp_ttc_result();
		$ret = $req->execute($result);
		$this->result = $result;

		//TODO ...
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = $errCode;
			//$this->errMsg  = "dtc config:{$this->dtcKey} delete:{$key} error,error code:{$errCode}";
			$this->errMsg = $result->error_message();
			$this->errFrom = $result->error_from();		
			return false;
		}
		return true;
	}

	public function getAffectRows()
	{
		if ( !empty( $this->result ) )
		{
			return $this->result->affected_rows();
		}
		else
		{
			return -1;
		}
	}

	/**
	 * 获得DTC记录
	 *
	 * @param   string  $key, 数据库的主键
	 * @param   array   $multikey, 可选参数, 多字段key时必选, 形如array('field2' => 1, 'field3' => 2)
	 * @param   array    $need, 可选参数, 默认返回所有列，也可以指明需要返回的列
	 * @param   int    $start, 可选参数, 默认从第一条符合条件的记录开始返回，也可指定返回记录的起始偏移量
	 * * @param   int    $need, 可选参数, 默认返回所有符合条件的记录，也可指明需要的条数
	 */
	public function get($key, $multikey = array(), $need = array(), $itemLimit = 0, $start = 0)
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		if ($this->checkKey($key) == false) {
			return false;
		}
		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_GET);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($key);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($key);
		} else {
			$this->errCode = 20020;
			$this->errMsg  = 'request failed';

			return false;
		}

		if($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} get:{$key}: set_key error";

			return false;
		}
		/*当前版本的dtc暂不支持多key的需求
		if ( !empty($multikey) && is_array($multikey) ) {
			// 处理多字段key情况
			foreach ( $multikey as $mk => $mv ) {
				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}
				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20028;
					$this->errMsg  = "dtc config:{$this->dtcKey} get:{$key}: set multikey error";

					return false;
				}
			}
		}*/
		$fields = $this->config['FIELDS'];
		var_dump($fields);
		$keyfield   = $this->config['KEY'];

		if (empty($need)) {

			foreach ($fields as $k => $v){
				if ($keyfield == $k){
					continue;
				}
				$errCode = $req->need($k);
				if($errCode != 0){
					$this->errCode = $errCode;
					$this->errMsg  = "dtc config:{$this->dtcKey} need({$k}) error,error code:{$errCode}";

					return false;
				}
			}
		}
		else
		{
			foreach ($need as $k) {
				if ($keyfield == $k){
					continue;
				}
				$errCode = $req->need($k);
				if($errCode != 0){
					$this->errCode = $errCode;
					$this->errMsg  = "dtc config:{$this->dtcKey} need({$k}) error,error code:{$errCode}";

					return false;
				}
			}
		}
		if (0 != $itemLimit) {
			$errCode = $req->limit($start, $itemLimit);
			if ($errCode != 0) {
				$this->dtcCode = $errCode;
				$this->errCode = $errCode;
				$this->errMsg  = "dtc config:{$this->dtcKey} get:{$key} error,error code:{$errCode}";
				return false;
			}

		}
		$result = new tphp_ttc_result();
		$req->execute($result);
		$this->result = $result;
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = $errCode;
			//$this->errMsg  = "dtc config:{$this->dtcKey} get:{$key} error,error code:{$errCode}";
			$this->errMsg = $result->error_message();
			$this->errFrom = $result->error_from();			
			return false;
		}
		$rowCount = $result->num_rows();
		$multiRows = array();
		for ($i = 0 ; $i < $rowCount; ++$i)
		{
			$ret = $result->fetch_row();
			if ($ret < 0)
			{
				$this->errCode = 20026;
				$this->errMsg  = "dtc config:{$this->dtcKey} get:{$key} error,fetch_row error";
				return false;
			}
			$rData = array();
			if (empty($need)) {
				foreach ($fields as $k => $v){
					if ($k == $keyfield) {
						continue;
					}
					switch ($v['type']){
						case DTC::INT :
							$rData[$k] = $result->int_value($k);
							break;
						case DTC::STRING :
							$rData[$k] = $result->string_value($k);
							break;
						case DTC::BINARY :
							$rData[$k] = $result->binary_value($k);
							break;
						case DTC::FLOAT  :
							$rData[$k] = $result->float_value($k);
							break;
						default:
							$this->errCode = 20027;
							$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} data type:{$v['type']} error";

							return false;
					}
				}
				$rData[$keyfield] = $key;
				$multiRows[] = $rData;
			}
			else
			{
				foreach ($need as $k){
					if ($k == $keyfield) {
						continue;
					}
					switch ($fields[$k]['type']){
						case DTC::INT :
							$rData[$k] = $result->int_value($k);
							break;
						case DTC::STRING :
							$rData[$k] = $result->string_value($k);
							break;
						case DTC::BINARY :
							$rData[$k] = $result->binary_value($k);
							break;
						case DTC::FLOAT  :
							$rData[$k] = $result->float_value($k);
							break;
						default:
							$this->errCode = 20027;
							$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} data type:{$v['type']} error";

							return false;
					}
				}
				$rData[$keyfield] = $key;
				$multiRows[] = $rData;
			}
		}

		return $multiRows;
	}

	/**
	 * 更新一条数据库记录
	 *
	 * @param $data    数据
	 */
	public function update($data, $multikey = array())
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		$key = $this->config['KEY'];
		if (!isset($data[$key])) {
			$this->errCode = 20031;
			$this->errMsg  = "dtc config:{$this->dtcKey} update key not set";

			return false;
		}
		if (($this->check($data) == false)) {
			return false;
		}

		$fields = $this->config['FIELDS'];
		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_UPDATE);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($data[$key]);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($data[$key]);
		} else {
			$this->errCode = 20020;
			$this->errMsg  = 'request failed';

			return false;
		}

		if($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} update:{$key}: set_key error";

			return false;
		}

		//当前版本暂不支持多key的需求
		/*if ( !empty($multikey) && is_array($multikey) ) {
			// 处理多字段key情况
			foreach ( $multikey as $mk => $mv ) {
				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}
				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20036;
					$this->errMsg  = "dtc config:{$this->dtcKey} update:{$key}: set multikey error";

					return false;
				}
			}
		}*/
		foreach ($data as $k => $v){
			if ($k == $key or (is_array($multikey) and array_key_exists($k, $multikey))) {
				continue;
			}
			if ( !isset($fields[$k]) ) {
				continue;
			}
			$tmpConfig = $fields[$k];
			switch($tmpConfig['type']){
				case DTC::INT:
					$data[$k] = $data[$k] + 0; // 确保类型正确,否则dtc扩展报warnning
					$req->set($k, $data[$k]);
					break;
				case DTC::STRING:
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_str($k, $data[$k]);
					break;
				case DTC::BINARY:
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_bin($k, $data[$k]);
					break;
				case DTC::FLOAT :
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_flo($k, $data[$k]);
					break;
				default:
					$this->errCode = 20033;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} data type:{$v['type']} error";

					return false;
			}
		}

		$result = new tphp_ttc_result();
		$req->execute($result);
		$this->result = $result;
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = 20034;
			//$this->errMsg  = "dtc config:{$this->dtcKey} update:{$data[$key]} error,error code:{$errCode}";
			$this->errMsg = $result->error_message();
			$this->errFrom = $result->error_from();		
			return false;
		}

		return true;
	}

	/**
	 * 增加一条数据库记录
	 *
	 * @param data    需要insert 的数据
	 */
	public function insert($data)
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		if ($this->check($data) == false) {
			return false;
		}

		$key = $this->config['KEY'];
		$fields = $this->config['FIELDS'];
		$keyConfig = $fields[$key];
		if (!isset($data[$key]) && $keyConfig['auto'] == false) {
			$this->errCode = 20041;
			$this->errMsg  = "dtc config:{$this->dtcKey} insert key not set";

			return false;
		}

		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_INSERT);
		if (isset($data[$key])) {
			if ( $this->key_type == 1 ) {
				$errCode = $req->set_key($data[$key]);
			} elseif ( $this->key_type == 2 ) {
				$errCode = $req->set_key_str($data[$key]);
			} else {
				$this->errCode = 200106;
				$this->errMsg  = 'request failed';

				return false;
			}

			if($errCode != 0){
				$this->errCode = $errCode;
				$this->errMsg  = "dtc config:{$this->dtcKey} insert:{$key}: set_key error";

				return false;
			}
		}

		foreach ($data as $k => $v){
			if ($k == $key) {
				continue;
			}
			if ( !isset($fields[$k]) ) {
				continue;
			}
			$tmpConfig = $fields[$k];
			switch($tmpConfig['type']){
				case DTC::INT:
					$data[$k] = $data[$k] + 0; // 确保类型正确,否则dtc扩展报warnning
					$req->set($k, $data[$k]);
					break;
				case DTC::STRING:
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_str($k, $data[$k]);
					break;
				case DTC::BINARY:
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_bin($k, $data[$k]);
					break;
				case DTC::FLOAT :
					$data[$k] = $data[$k] . ''; // 确保类型正确,否则dtc扩展报warnning
					$req->set_flo($k, $data[$k]);
					break;
				default:
					$this->errCode = 20043;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} data type:{$v['type']} error";

					return false;
			}
		}

		$result = new tphp_ttc_result();
		$req->execute($result);
		$this->result = $result;
		//TODO...
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = $errCode;
			$k_val = isset($data[$key]) ? $data[$key] : '[auto_increment]';
			//$this->errMsg  = "dtc config:{$this->dtcKey} insert:{$k_val} error,code:{$errCode}";
			$this->errMsg = $result->error_message();
			$this->errFrom = $result->error_from();		
			return false;
		}

		// 若为自增型key，返回insert_id
		if ( !isset($data[$key]) && $keyConfig['auto'] ) {
			$autoinc_id = $result->int_key();
			return $autoinc_id;
		}

		return true;
	}

	/**
	 * 整形字段自增
	 *
	 * @param   array   $key_arr, key数组
	 * @param   string  $fld, 字段名,必须为整形字段
	 * @param   int     $val, 自增值
	 * @return  boolean
	 */
	public function increment($key_arr, $fld, $val = 1)
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		$key = $this->config['KEY'];
		if (!isset($key_arr[$key])) {
			$this->errCode = 20071;
			$this->errMsg  = "dtc config:{$this->dtcKey} increment key not set";

			return false;
		}
		$fields = $this->config['FIELDS'];
		if ( !array_key_exists($fld, $fields) ) {
			$this->errCode = 20072;
			$this->errMsg  = "dtc config:{$this->dtcKey} increment no field name $fld";

			return false;
		}
		$val = intval($val);
		if ( $val < 1 ) {
			$this->errCode = 20073;
			$this->errMsg  = "dtc config:{$this->dtcKey} invalid increment value";

			return false;
		}
		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_UPDATE);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($key_arr[$key]);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($key_arr[$key]);
		} else {
			$this->errCode = 200110;
			$this->errMsg  = 'request failed';

			return false;
		}
		if($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} increment:{$key}: set_key error";
			return false;
		}
		/*当前版本的dtc暂不支持多key的需求
		if ( count($key_arr) > 1 ) {
			// 处理多字段key情况
			foreach ( $key_arr as $mk => $mv ) {
				if ($mk == $key) {
					continue;
				}
				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}
				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20075;
					$this->errMsg  = "dtc config:{$this->dtcKey} increment:{$key}: set multikey error";

					return false;
				}
			}
		}*/

		$add = $req->add($fld, $val);
		if ( $add != 0 ) {
			$this->errCode = 20076;
			$this->errMsg  = "dtc config:{$this->dtcKey} increment:{$key}: add error";

			return false;
		}

		$result = new tphp_ttc_result();
		$req->execute($result);
		$this->result = $result;
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = 20077;
			$this->errMsg  = "dtc config:{$this->dtcKey} increment:{$key} error,error code:{$errCode}";

			return false;
		}

		return true;
	}

	/**
	 * 整形字段自减
	 *
	 * @param   array   $key_arr, key数组
	 * @param   string  $fld, 字段名,必须为整形字段
	 * @param   int     $val, 自减值
	 * @return  boolean
	 */
	public function decrement($key_arr, $fld, $val = 1)
	{
		$this->clearERR();
		if (($this->server == false) && ($this->init() == false)) {
			return false;
		}

		$key = $this->config['KEY'];
		if (!isset($key_arr[$key])) {
			$this->errCode = 20081;
			$this->errMsg  = "dtc config:{$this->dtcKey} decrement key not set";

			return false;
		}

		$fields = $this->config['FIELDS'];
		if ( !array_key_exists($fld, $fields) ) {
			$this->errCode = 20082;
			$this->errMsg  = "dtc config:{$this->dtcKey} decrement no field name $fld";

			return false;
		}

		$val = intval($val);
		if ( $val < 1 ) {
			$this->errCode = 20083;
			$this->errMsg  = "dtc config:{$this->dtcKey} invalid decrement value";

			return false;
		}

		$req = new tphp_ttc_request($this->server, TPHP_DTC_OP_UPDATE);
		if ( $this->key_type == 1 ) {
			$errCode = $req->set_key($key_arr[$key]);
		} elseif ( $this->key_type == 2 ) {
			$errCode = $req->set_key_str($key_arr[$key]);
		} else {
			$this->errCode = 200111;
			$this->errMsg  = 'request failed';

			return false;
		}

		if ($errCode != 0){
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} decrement:{$key}: set_key error";

			return false;
		}

		if ( count($key_arr) > 1 ) {
			// 处理多字段key情况
			foreach ( $key_arr as $mk => $mv ) {
				if ($mk == $key) {
					continue;
				}

				$chk = $this->checkMultikey($mk, $mv);
				if ( !$chk ) {
					return $chk;
				}

				$eq = $this->eqv($req, $mk, $mv);
				if ( $eq != 0 ) {
					$this->errCode = 20085;
					$this->errMsg  = "dtc config:{$this->dtcKey} decrement:{$key}: set multikey error";

					return false;
				}
			}
		}

		$add = $req->sub($fld, $val);
		if ( $add != 0 ) {
			$this->errCode = 20086;
			$this->errMsg  = "dtc config:{$this->dtcKey} decrement:{$key}: sub error";

			return false;
		}

		$result = new tphp_ttc_result();
		$req->execute($result);
		$this->result = $result;
		$errCode = $result->result_code();
		if ($errCode != 0) {
			$this->dtcCode = $errCode;
			$this->errCode = $errCode;
			$this->errMsg  = "dtc config:{$this->dtcKey} decrement:{$key} error,error code:{$errCode}";

			return false;
		}

		return true;
	}

	/**
	 * 检测数据是否合法
	 *
	 * @param data    数据
	 * @param type    1:insert, 2:update 3:delete 4:get
	 */
	public function check(&$data)
	{
		$this->clearERR();
		$fields = $this->config['FIELDS'];
		foreach ($fields as $k => $v){
			if (!isset($data[$k])) {
				continue;
			}
			switch ($v['type']){
				case DTC::INT :
				case DTC::FLOAT :
					if ($v['min'] > $data[$k] || $data[$k] > $v['max']) {
						$this->errCode = 20051;
						$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} error";
						return false;
					} else {
						$data[$k] = 0 + $data[$k];
					}
					break;
				case DTC::STRING :
					$len = strlen($data[$k]);
					if ($v['min'] > $len || $len > $v['max']) {
						$this->errCode = 20052;
						$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} error";
						return false;
					} else {
						$data[$k] = '' . $data[$k];
					}
					break;
				case DTC::BINARY :
					$len = strlen($data[$k]);
					if ($v['min'] > $len || $len > $v['max']) {
						$this->errCode = 20053;
						$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} error";
						return false;
					} else {
						$data[$k] = '' . $data[$k];
					}
					break;
				default:
					$this->errCode = 20054;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$k} data type:{$v['type']} error";
					return false;
			}
		}
		return true;
	}

	/**
	 * 检测数据是否合法
	 *
	 * @param key    表的主键
	 */
	public function checkKey(&$key)
	{
		$this->clearERR();
		$v = $this->config['FIELDS'][$this->config['KEY']];
		switch($v['type']){
			case DTC::INT:
			case DTC::FLOAT :
				$ret = ($v['min'] <= $key && $key <= $v['max']);
				if($ret == false){
					$this->errCode = 20061;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} error";
				} else {
					$key = 0 + $key;
				}
				return $ret;
			case DTC::STRING:
				$len = strlen($key);
				$ret = ($v['min'] <= $len && $len <= $v['max']);
				if ($ret === false) {
					$this->errCode = 20062;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} error";
				} else {
					$key = '' . $key;
				}
				return $ret;
			case DTC::BINARY:
				//TODO...
				return true;
			default:
				$this->errCode = 20063;
				$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} data type:{$v['type']} error";
				return false;
		}
	}
/* 当前版本的dtc不支持多key的需求
	/**
	 * 检测多字段key情况下，除主key外的其他key字段
	 *
	 * @param   string  $key, key字段名
	 * @param   mix     $val, key字段值(引用), 若检测通过, 根据字段类型同时进行类型转换, 避免参数类型错误
	 * @return  boolean
	 */
	 /*
	private function checkMultikey($key, &$val)
	{
		if ( !isset($this->config['FIELDS'][$key]) ) {
			$this->errCode = 20064;
			$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} not exists";
			return false;
		}
		$field = $this->config['FIELDS'][$key];
		switch($field['type']){
			case DTC::INT:
			case DTC::FLOAT :
				$ret = ($field['min'] <= $val && $val <= $field['max']);
				if($ret == false){
					$this->errCode = 20065;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} error";
				} else {
					$val = 0 + $val;
				}
				return $ret;
			case DTC::STRING:
				$len = strlen($val);
				$ret = ($field['min'] <= $len && $len <= $field['max']);
				if ($ret === false) {
					$this->errCode = 20066;
					$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} error";
				} else {
					$val = '' . $val;
				}
				return $ret;
			case DTC::BINARY:
				//TODO...
				return true;
			default:
				$this->errCode = 20067;
				$this->errMsg  = "dtc config:{$this->dtcKey} field:{$key} data type:{$field['type']} error";
				return false;
		}
	}
*/
	/**
	 * 为request对象添加请求条件(通常为多字段key情况下，附加其他key)
	 *
	 * @param   tphp_ttc_request    $req, dtc请求对象
	 * @param   string              $key, 字段名
	 * @param   mix                 $val, 字段值
	 * @return  int                 $ret
	 */
	private function eqv(&$req, $key, $val) {
		if ( is_string($val) ) {
			$ret = $req->eq_str($key, $val);
		} elseif ( is_numeric($val) ) {
			$ret = $req->eq($key, $val);
		}

		return $ret;
	}

	/**
	 * 清除错误标识，在每个函数调用前调用
	 */
	private function clearERR()
	{
		$this->errCode = 0;
		$this->errMsg  = '';
		$this->errFrom = '';
		$this->dtcCode = 0;
	}
}


//End Of Script
