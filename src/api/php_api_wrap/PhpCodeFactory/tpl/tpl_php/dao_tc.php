<?php
/**
 * I{TABLE_NAME_FB}TTC.php
 * TTC: {TABLE_NAME_B} 的增、查、删、改等操作
 * 数据类型: int=1,string=2,binary=3,float=4
 * 
 * @author (DTC Team)
 */
require_once('DTC.php');

global $_DTC_CFG;

$_DTC_CFG['{TABLE_NAME}']['TTCKEY'] = '{TTCKEY}';
$_DTC_CFG['{TABLE_NAME}']['TABLE']  = '{TABLE_NAME}';
$_DTC_CFG['{TABLE_NAME}']['IP']  = '{IP}';
$_DTC_CFG['{TABLE_NAME}']['PORT']  = '{PORT}';
$_DTC_CFG['{TABLE_NAME}']['TOKEN']  = '{TOKEN}';
$_DTC_CFG['{TABLE_NAME}']['TimeOut']= 1;
$_DTC_CFG['{TABLE_NAME}']['KEY']    = '{PRIKEY}';
$_DTC_CFG['{TABLE_NAME}']['FIELDS'] = array(
<!-- BEGIN FIELD1 -->
	{FIELD1_NAME} => array('type' => {FIELD1_TYPE},'min' => {MIN_NUM},'max' => {MAX_NUM}),
<!-- END FIELD1 -->
);

class I{TABLE_NAME}DTC
{
	/**
	 * 错误编码
	 */
	public static $errCode = 0;
	/**
	 * 错误消息
	 */
	public static $errMsg  = '';
	/**
	 * 清除错误标识,在每个函数调用前调用
	 */
	private static function clearErr()
	{
		self::$errCode = 0;
		self::$errMsg  = '';
	}

	/**
	 * 增加一条TTC记录
	 * @param it 记录数据,格式: 
	 * 	array(
<!-- BEGIN FIELD2 -->
	 * 		{FIELD2_NAME} => {FIELD2_VALUE},//{FIELD2_NAME_CH}
<!-- END FIELD2 -->
	 * 		)
	 * 
	 * 返回值:正确返回true,错误返回false
	 */
	public static function insert($it)
	{
		self::clearErr();
		if(empty($it) || !is_array($it)){
			self::$errCode = 111;
			self::$errMsg  = __FILE__ ."\t". __LINE__ .' it is empty';
			return false;
		}
		$dtc = DTCManager::getDTC('{TABLE_NAME}');
		if(!$dtc){
			self::$errCode = DTCManager::$errCode;
			self::$errMsg  = DTCManager::$errMsg;
			return false;
		}
		$rt = $dtc->insert($it);
		if(false === $rt){
			self::$errCode = $dtc->errCode;
			self::$errMsg  = $dtc->errMsg;
			return false;
		}
		return $rt;
	}

	/**
	 * 更新一条TTC记录
	 * @param it 记录数据,格式: 
	 * 	array(
{FIELD2S}	 * 		)
	 * @param where 过滤条件,数组存储 
	 * 
	 * 返回值:正确返回true,错误返回false
	 */
	public static function update($it,$where=array())
	{
		self::clearErr();
		if(empty($it) || !is_array($it)){
			self::$errCode = 111;
			self::$errMsg  = __FILE__ ."\t". __LINE__ .' it is empty';
			return false;
		}
		$dtc = DTCManager::getDTC('{TABLE_NAME}');
		if(!$dtc){
			self::$errCode = DTCManager::$errCode;
			self::$errMsg  = DTCManager::$errMsg;
			return false;
		}
		$rt = $dtc->update($it,$where);
		if(false === $rt){
			self::$errCode = $dtc->errCode;
			self::$errMsg  = $dtc->errMsg;
			return false;
		}
		return $rt;
	}

	/**
	 * 删除一条TTC记录
	 * @param key 数据库的主键
	 * @param where 过滤条件,数组存储 
	 * 
	 * 返回值:正确返回true,错误返回false
	 */
	public static function remove($key,$where=array())
	{
		self::clearErr();
		if(empty($key)){
			self::$errCode = 111;
			self::$errMsg  = __FILE__ ."\t". __LINE__ .' key is empty';
			return false;
		}
		$dtc = DTCManager::getDTC('{TABLE_NAME}');
		if(!$dtc){
			self::$errCode = DTCManager::$errCode;
			self::$errMsg  = DTCManager::$errMsg;
			return false;
		}
		$rt = $dtc->delete($key,$where);
		if(false === $rt){
			self::$errCode = $dtc->errCode;
			self::$errMsg  = $dtc->errMsg;
			return false;
		}
		return $rt;
	}

	/**
	 * 取TTC记录
	 * @param key    数据库的主键
	 * @param where  过滤条件,多字段key时必选,形如array('field2' => 1,'field3' => 2)
	 * @param fields 可选参数,默认返回所有列,也可以指明需要返回的列
	 * @param itNum  可选参数,记录条数,默认全区
	 * @param start  可选参数,默认从第一条符合条件的记录开始返回,也可指定返回记录的起始偏移量
	 * 
	 * 返回值:正确返回数据,错误返回false
	 * 数据格式:
	 * 	array(
{FIELD2S}	 * 		)
	 */
	public static function get($key,$where=array(),$fields=array(),$itNum=0,$start=0)
	{
		self::clearErr();
		if(empty($key)){
			self::$errCode = 111;
			self::$errMsg  = __FILE__ ."\t". __LINE__ .' key is empty';
			return false;
		}
		$dtc = DTCManager::getDTC('{TABLE_NAME}');
		if(!$dtc){
			self::$errCode = DTCManager::$errCode;
			self::$errMsg  = DTCManager::$errMsg;
			return false;
		}
		$itLst = $dtc->get($key,$where,$fields,$itNum,$start);
		if(false === $itLst){
			self::$errCode = $dtc->errCode;
			self::$errMsg  = $dtc->errMsg;
			return false;
		}
		return $itLst;
	}


	/**
	 * 取操作TTC影响的行数
	 * 返回值:正确返回>-1的行数,错误返回负数
	 */
	public static function getDTCAffectRows()
	{
		self::clearErr();
		$dtc = DTCManager::getDTC('{TABLE_NAME}');
		if(!$dtc){
			self::$errCode = DTCManager::$errCode;
			self::$errMsg  = DTCManager::$errMsg;
			return -1;
		}
		return $dtc->getAffectRows();
	}

}

//End Of Script

