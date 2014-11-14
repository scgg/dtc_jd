package com.jd.dtc.template;
import java.util.ArrayList;
import java.util.List;

import com.jd.dtc.template.DTC{TableName};

public class I{TableName}DTC
{
	public int errorCode;
	public int dtcRetCode;
	public String errorMsg;
	public String errorFrom;
	public DTC{TableName} dtc =null;
	
	public I{TableName}DTC()
	{
		this.dtc = new DTC{TableName}();
	}
	
	/*
	 * @attention: 在程序结束之前需要调用该方法，释放 dtc api Server 对象，否者会导致内存泄漏
	 */
	public void DestroyServer()
	{
		this.dtc.DestroyServer();
	}
	
	/*
	 * @attention: 在程序结束之前需要调用该方法，释放 dtc api Request 对象，否者会导致内存泄漏
	 */
	public void DestroyRequest()
	{
		this.dtc.DestroyRequest();
	}
	
	public void clearErr()
	{
		this.dtcRetCode = 0;
		this.errorCode = 0;
		this.errorMsg = "";
		this.errorFrom = "";
	}
	
	/*
	 * @describe 清理缓存
	 *
	 * @param key dtc key 字段值(fieldvalue)
	 *
	 * @return boolean
	 */
	public boolean purge(String key)
	{
		clearErr();
		boolean ret = dtc.purge(key);
		
		if(ret == false)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		return ret;
	}
	
	/*
	 * @describe 删除一条数据库记录
	 *
	 * @param key    dtc key 字段值(fieldvalue)
	 */
	public boolean delete(String key)
	{
		clearErr();
		boolean ret = dtc.delete(key);
		
		if(ret == false)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		return ret;
	}
	
	/*
	 * 获得DTC记录
	 * @param   string  key dtc key字段值(fieldvalue)
	 * @param   List<Field>   need, 可选参数, 默认返回所有列，也可以指明需要返回的列
	 * @atention 当指定need 条件时，必须传入Field类对象中的fieldName以及fieldType两个参数
	 */
	public List<Field> get( String key, List<Field> need)
	{
		clearErr();
		List<Field> result = dtc.get(key, need);
		if(dtc.dtcCode != 0)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		if(result == null)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		return result;
	}
	
	/*
	 * @describe: 更新一条数据库记录
	 * @param:  key dtc key 字段值(fieldvalue)
	 * @param List<Field> fieldList 需要更新的字段集，每个子集Field必须指定 fieldName,fieldType以及fieldValue三个参数
	 */
	public boolean update(String key, List<Field> fieldList)
	{
		clearErr();
		boolean ret = dtc.update(key, fieldList);
		
		if(ret == false)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		return ret;
	}
	
	/*
	 * @describe 增加一条数据库记录
	 *
	 * @param:  key dtc key 字段值(fieldvalue)
	 * @param List<Field> fieldList 需要插入的字段集，每个子集Field必须指定 fieldName以及fieldType,fieldValue三个参数
	 */
	public boolean insert(String key, List<Field> fieldList)
	{
		clearErr();
		boolean ret = dtc.insert(key, fieldList);
		
		if(ret == false)
		{
			this.dtcRetCode = dtc.dtcCode;
			this.errorCode = dtc.errCode;
			this.errorMsg = dtc.errMsg;
			this.errorFrom = dtc.errFrom;
		}
		return ret;
	}
	
}// end of class
