package com.jd.dtc.template;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.jd.dtc.api.Result;
import com.jd.dtc.api.Request;
import com.jd.dtc.api.Server;
import com.jd.dtc.template.Field;

/*
 * @describe 封装DTC的一些API
 * @author (DTC Team)
 * @date 20140820
 */
public class DTCt_dtc_example
{

	public class OperType
	{
		public static final int TJAVA_DTC_OP_GET     =  4;
		public static final int TJAVA_DTC_OP_PURGE   =  5;
		public static final int TJAVA_DTC_OP_INSERT  =  6;
		public static final int TJAVA_DTC_OP_UPDATE  =  7;
		public static final int TJAVA_DTC_OP_DELETE  =  8;
		public static final int TJAVA_DTC_OP_REPLACE =  12;
		public static final int TJAVA_DTC_OP_FLUSH   =  13;
		public static final int TJAVA_DTC_OP_INVALID =  14;
	
	};
		
	public class DTCErrorCode
	{
		public static final int E_SET_TTIMEOUT_FAIL = 20000;
		public static final int E_INVALID_KEY_TYPE	= 20001;
		public static final int E_INVALID_FIELD_TYPE = 20002;
		public static final int E_INVALID_FIELD_VALUE = 20003;
		public static final int E_INVALID_KEY_VALUE = 20004;
		public static final int E_KEY_MISMATCH = 20005;
		public static final int E_EMPTY_FIELDNAME = 20006;
		public static final int E_INVALID_FIELDNAME = 20007;
	}
	
	public static class DTCConfig
	{
		public static final String tablename = "t_dtc_example";
		public static final String dtckey = "uid";
		public static final String ip = "1.1.1.2";
		public static final String port = "10901";
		public static final String token = "0000090184d9cfc2f395ce883a41d7ffc1bbcf4e";
		public static final int timeout = 5;	
		public static final String[][] fieldcfg = {
		
				{"uid", "1", "-2147483648", "2147483647"},
		
				{"name", "4", "0", "100"},
		
				{"city", "4", "0", "100"},
		
				{"descr", "5", "0", "255"},
		
				{"age", "1", "-2147483648", "2147483647"},
		
				{"salary", "1", "-2147483648", "2147483647"},
				};
	};
	
	/*
	 * 错误码
	 */
	public int errCode;
	/*
	 * 错误信息
	 */
	public String errMsg;
	/*
	 * 错误来源
	 */
	public String errFrom;
	
	/*
	 * DTC返回码
	 */
	public int dtcCode;
	
	public int affectRow;

	/*
	 * dtc key 的类型，只能是这些类型：Signed=1, Unsigned=2, String=4, Binary=5
	 */
	private int key_type;
	
	private int keyIndex;

	/*
	 * dtc数据连接
	 */
	private Server dtcServer = null;
	private Request insertReq = null;
	private Request getReq = null;
	private Request deleteReq = null;
	private Request updateReq = null;
	private Request purgeReq = null;
	private String dtcKey = "";
	
	/*
	 * 初始化dtc的配置,这个只有在真正操作的时候才会去做
	 *
	 * @return boolean
	 */
	
	public DTCt_dtc_example()
	{
		init();
	}
	
	private void init()
	{
		int ret = 0;
		clearERR();
		if (dtcServer == null) 
		{
			dtcServer = new Server(0);
			this.dtcKey = DTCConfig.dtckey;
			dtcServer.SetTimeout(DTCConfig.timeout);
			if (errCode != 0) {
				this.errCode = DTCErrorCode.E_SET_TTIMEOUT_FAIL;
				this.errMsg  = dtcServer.ErrorMessage();
				dtcServer.Close();
			}
			ret = dtcServer.SetTableName(DTCConfig.tablename);
			if (0 != ret) 
			{
				this.errCode = ret;
				this.errMsg  = dtcServer.ErrorMessage();
				dtcServer.Close();
			}
			ret = dtcServer.SetAddress(DTCConfig.ip, DTCConfig.port);
			if (0 != ret) 
			{
				this.errCode = ret;
				this.errMsg  = dtcServer.ErrorMessage();
				dtcServer.Close();
			}
			dtcServer.SetAccessKey(DTCConfig.token);			
			// 此处从DTCConfig.java 中找出key所在数组，找到key type
			int keyflag = 0;
			for(int i=0; i< DTCConfig.fieldcfg.length;i++)
			{
				if(DTCConfig.fieldcfg[i][0] == DTCConfig.dtckey)
				{
					keyflag = i;
					break;
				}
			}			
			key_type = Integer.parseInt(DTCConfig.fieldcfg[keyflag][1]);
			keyIndex = keyflag;
			if ( key_type == 1 || key_type == 2)
			{
				this.errCode = dtcServer.IntKey();
			}
			else if (this.key_type == 4 ) 
			{
				this.errCode = dtcServer.StringKey();
			}
			else if (this.key_type == 5 ) 
			{
				this.errCode = dtcServer.BinaryKey();
			}
			else  
			{
				this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
				this.errMsg  = "invalid key type";
				dtcServer.Close();
			}
			if (errCode != 0) {
				this.errMsg  = dtcServer.ErrorMessage();
				dtcServer.Close();
			}
		}
		if(insertReq == null)
		{
			this.insertReq = new Request(dtcServer, OperType.TJAVA_DTC_OP_INSERT);
		}
		if(getReq == null)
		{
			this.getReq = new Request(dtcServer, OperType.TJAVA_DTC_OP_GET);
		}
		if(deleteReq == null)
		{
			this.deleteReq = new Request(dtcServer, OperType.TJAVA_DTC_OP_DELETE);
		}
		if(updateReq == null)
		{
			this.updateReq = new Request(dtcServer, OperType.TJAVA_DTC_OP_UPDATE);
		}
		if(purgeReq == null)
		{
			this.purgeReq = new Request(dtcServer, OperType.TJAVA_DTC_OP_PURGE);
		}
	}
	
	/*
	 * @describe 清除server对象
	 * @void
	 */
	public void DestroyServer()
	{
		this.dtcServer.DestroyServer();
		this.dtcServer = null;
	}
	
	public void DestroyRequest()
	{
		this.getReq.DestroyRequest();
		this.getReq = null;
		this.updateReq.DestroyRequest();
		this.updateReq = null;
		this.insertReq.DestroyRequest();
		this.insertReq = null;
		this.deleteReq.DestroyRequest();
		this.deleteReq = null;
		this.purgeReq.DestroyRequest();
		this.purgeReq = null;
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
		int ret = 0;
		clearERR();
		this.purgeReq.Reset();
		if(dtcServer == null) 
		{
			return false;
		}
		if (checkKey(key) == false)
		{
			return false;
		}
		if ( key_type == 1 || key_type == 2)
		{
			ret = purgeReq.SetKey(Long.parseLong(key));
		} 
		else if ( key_type == 4 || key_type == 5 ) 
		{
			ret = purgeReq.SetKey(key, key.length());
		} 
		else 
		{
			this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
			this.errMsg  = "purge request failed: bad keyType!";
			return false;
		}
		if(0 != ret)
		{
			this.errCode = ret;
			this.errMsg  = "##dtc## purge setkey fail!";
			return false;
		}
		Result rst = new Result();
		ret = purgeReq.Execute(rst);
		if (0 != ret) 
		{
			this.dtcCode = rst.ResultCode();
			this.errCode = ret;
			this.errMsg = rst.ErrorMessage();
			this.errFrom = rst.ErrorFrom();
			rst.DestroyResult();
			rst = null;
			return false;
		}
		this.affectRow = rst.AffectedRows();
		rst.DestroyResult();
		rst = null;
		return true;
	}

	/*
	 * @describe 删除一条数据库记录
	 *
	 * @param key    dtc key 字段值(fieldvalue)
	 */
	public boolean delete(String key)
	{
		int ret = 0;
		clearERR();
		this.deleteReq.Reset();
		if (dtcServer == null)
		{
			return false;
		}
		if (checkKey(key) == false)
		{
			return false;
		}
		if (key_type == 1 || key_type == 2)
		{
			ret = deleteReq.SetKey(Long.parseLong(key));
		}
		else if(this.key_type == 4 || this.key_type == 5 ) 
		{
			ret  = deleteReq.SetKey(key, key.length());
		} 
		else 
		{
			this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
			this.errMsg  = "delete request failed: bad keyType!";
			return false;
		}
		if(0 != ret)
		{
			this.errCode = ret;
			this.errMsg  = "##dtc## delete setkey fail!";
			return false;
		}
		Result rst = new Result();
		ret = deleteReq.Execute(rst);
		if (0 != ret) 
		{
			this.dtcCode = rst.ResultCode();
			this.errCode = ret;
			this.errMsg = rst.ErrorMessage();
			this.errFrom = rst.ErrorFrom();
			rst.DestroyResult();
			rst = null;
			return false;
		}
		this.affectRow = rst.AffectedRows();
		rst.DestroyResult();
		rst = null;
		return true;
	}

	/*
	 * 获得DTC记录
	 * @param   string  key, dtc key 字段值(fieldvalue)
	 * @param   List<Field>   need, 可选参数, 默认返回所有列，也可以指明需要返回的列
	 * @atention 当指定need 条件时，必须传入Field类对象中的fieldName以及fieldType两个参数
	 */
	public List<Field> get( String key, List<Field> need)
	{
		int ret = 0;
		List<Field> resultSet = new ArrayList<Field>();
		clearERR();
		this.getReq.Reset();
		if(dtcServer == null)
		{
			return null;
		}
		if (checkKey(key) == false) 
		{
			return null;
		}
		if ( key_type == 1 || key_type == 2)
		{
			ret = getReq.SetKey(Long.parseLong(key));
		} 
		else if ( this.key_type == 4 || this.key_type == 5) 
		{
			ret = getReq.SetKey(key, key.length());
		} 
		else 
		{
			this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
			this.errMsg  = "get request failed: bad keyType!";
			return null;
		}
		if(0 != ret)
		{
			this.errCode = ret;
			this.errMsg  = "##dtc## get setkey fail!";
			return null;
		}
		if (need == null) {
			for(int i=0; i< DTCConfig.fieldcfg.length; ++i)
			{
				if(DTCConfig.fieldcfg[i][0] == dtcKey)
					continue;
				ret = getReq.Need(DTCConfig.fieldcfg[i][0]);
				if(0 != ret){
					this.errCode = ret;
					this.errMsg  = "##dtc## get Need error without need list!";
					return null;
				}
			}// end for
		}
		else
		{
			for(Iterator<Field> iter = need.iterator(); iter.hasNext(); ) 
			{
				Field fd = iter.next();
				if(fd.getFieldname() == "")
				{
					this.errCode = DTCErrorCode.E_EMPTY_FIELDNAME;
					this.errMsg  = "##dtc## get Need error---empty fieldname!";
					return null;
				}
				if(fd.getFieldname() == dtcKey)
					continue;
				ret = getReq.Need(fd.getFieldname());
				if(0 != ret){
					this.errCode = ret;
					this.errMsg  = "##dtc## get Need error with need list!";
					return null;
				}
			}// end for
		}
		
		// 调用dtc api 执行查询操作
		Result rst = new Result();
		ret = getReq.Execute(rst);	
		if (0 != ret) 
		{
			this.dtcCode = rst.ResultCode();
			this.errCode = ret;
			this.errMsg = rst.ErrorMessage();
			this.errFrom = rst.ErrorFrom();		
			rst.DestroyResult();
			rst = null;
			return null;
		}
		this.affectRow = rst.AffectedRows();
		int rowCount = rst.NumRows();
		// 拼装结果集
		for (int i = 0 ; i < rowCount; ++i)
		{
			ret = rst.FetchRow();
			if (ret < 0)
			{
				this.errCode = ret;
				this.errMsg  = rst.ErrorMessage();
				this.errFrom = rst.ErrorFrom();
				rst.DestroyResult();
				rst = null;
				return null;
			}
			// 不带need 条件，返回所有满足key条件的结果集
			if (need == null ) 
			{
				for(int j=0; j< DTCConfig.fieldcfg.length; ++j)
				{
					if(DTCConfig.fieldcfg[j][0] == this.dtcKey)
						continue;
					Field resultField = new Field();
					switch (Integer.parseInt(DTCConfig.fieldcfg[j][1]))
					{
					case 1:
						resultField.setFieldType(1);
						resultField.setFieldname(DTCConfig.fieldcfg[j][0]);
						resultField.setFieldvalue(Long.toString(rst.IntValue(DTCConfig.fieldcfg[j][0])));
						resultSet.add(resultField);
						break;
					case 2:
						resultField.setFieldType(2);
						resultField.setFieldname(DTCConfig.fieldcfg[j][0]);
						resultField.setFieldvalue(Long.toString(rst.IntValue(DTCConfig.fieldcfg[j][0])));
						resultSet.add(resultField);
						break;
					case 3:
						resultField.setFieldType(3);
						resultField.setFieldname(DTCConfig.fieldcfg[j][0]);
						resultField.setFieldvalue(Double.toString(rst.FloatValue(DTCConfig.fieldcfg[j][0])));
						resultSet.add(resultField);
						break;
					case 4:
						resultField.setFieldType(4);
						resultField.setFieldname(DTCConfig.fieldcfg[j][0]);
						resultField.setFieldvalue(rst.StringValue(DTCConfig.fieldcfg[j][0]));
						resultSet.add(resultField);
						break;
					case 5:
						resultField.setFieldType(5);
						resultField.setFieldname(DTCConfig.fieldcfg[j][0]);
						resultField.setFieldvalue(rst.BinaryValue(DTCConfig.fieldcfg[j][0]));
						resultSet.add(resultField);
						break;
					default:
						this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
						this.errMsg  = "##dtc## get invalid feildType without need condition!";
						rst.DestroyResult();
						rst = null;
						return null;
					}// end switch 
				}// end for
			}
			// 带有need条件，返回满足need 数组条件的结果集
			else
			{				
				for(Iterator<Field> iterNeed = need.iterator(); iterNeed.hasNext(); )
				{
					Field resultField = iterNeed.next();
					if(resultField.getFieldType() == 0)
					{
						this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
						this.errMsg  = "##dtc## get invalid fieldType, fieldType is 0!";
						rst.DestroyResult();
						rst = null;
						return null;
					}
					if(resultField.getFieldname() == this.dtcKey)
						continue;
					switch(resultField.getFieldType())
					{
					case 1:
						resultField.setFieldvalue(Long.toString(rst.IntValue(resultField.fieldname)));
						resultSet.add(resultField);
						break;
					case 2:
						resultField.setFieldvalue(Long.toString(rst.IntValue(resultField.fieldname)));
						resultSet.add(resultField);
						break;
					case 3:
						resultField.setFieldvalue(Double.toString(rst.FloatValue(resultField.fieldname)));
						resultSet.add(resultField);
						break;
					case 4:
						resultField.setFieldvalue(rst.StringValue(resultField.fieldname));
						resultSet.add(resultField);
						break;
					case 5:
						resultField.setFieldvalue(rst.BinaryValue(resultField.fieldname));
						resultSet.add(resultField);
						break;
					default:
						this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
						this.errMsg  = "##dtc## get invalid feildType with need condition!";
						rst.DestroyResult();
						rst = null;
						return null;
					}// end switch
				}// end for
			}
		}
		rst.DestroyResult();
		rst = null;
		return resultSet;
	}

	/*
	 * @describe: 更新一条数据库记录
	 * @param:  key dtc key 字段值(fieldvalue)
	 * @param List<Field> fieldList 需要更新的字段集，每个子集Field必须指定 fieldName,fieldType以及fieldValue三个参数
	 */
	public boolean update(String key, List<Field> fieldList)
	{
		int ret = 0;
		clearERR();
		this.updateReq.Reset();
		if(dtcServer == null )
		{
			return false;
		}
		if(checkKey(key) == false)
		{
			return false;
		}
		if ((check(fieldList) == false))
		{
			return false;
		}
		if ( key_type == 1 || key_type == 2)
		{
			ret = updateReq.SetKey(Long.parseLong(key));
		} 
		else if ( this.key_type == 4 || this.key_type == 5) 
		{
			ret = updateReq.SetKey(key, key.length());
		} 
		else 
		{
			this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
			this.errMsg  = "update request failed: bad keyType!";
			return false;
		}
		if(0 != ret)
		{
			this.errCode = ret;
			this.errMsg  = "##dtc## update setkey fail!";
			return false;
		}
		for(Iterator<Field> iter = fieldList.iterator(); iter.hasNext(); )
		{
			Field field = iter.next();
			if(field == null)
			{
				continue;
			}
			if(field.getFieldname() == this.dtcKey)
			{
				continue;
			}
			switch(field.getFieldType())
			{
			case 1:
				ret = updateReq.Set(field.getFieldname(), Integer.parseInt(field.getFieldvalue()));
				break;
			case 2:
				ret = updateReq.Set(field.getFieldname(), Integer.parseInt(field.getFieldvalue()));
				break;
			case 3:
				ret = updateReq.Set(field.getFieldname(), Double.parseDouble(field.getFieldvalue()));
				break;
			case 4:
				ret = updateReq.Set(field.getFieldname(), field.getFieldvalue());
				break;	
			case 5:
				ret = updateReq.Set(field.getFieldname(), field.getFieldvalue(), (field.getFieldvalue()).length());
				break;
			default:
				this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
				this.errMsg  = "##dtc## update invalid feildType when set fields!";
				return false;
			}// end switch
		}// end for
		
		// 调用dtc api 执行更新操作
		Result rst = new Result();
		ret = updateReq.Execute(rst);
		if (0 != ret)
		{
			this.dtcCode = rst.ResultCode();
			this.errCode = ret;
			this.errMsg = rst.ErrorMessage();
			this.errFrom = rst.ErrorFrom();	
			rst.DestroyResult();
			rst = null;
			return false;
		}
		this.affectRow = rst.AffectedRows();
		rst.DestroyResult();
		rst = null;
		return true;
	}

	/*
	 * @describe 增加一条数据库记录
	 *
	 * @param:  key dtc key 字段值(fieldvalue)
	 * @param List<Field> fieldList 需要插入的字段集，每个子集Field必须指定 fieldName以及fieldType,fieldValue三个参数
	 */
	public boolean insert(String key, List<Field> fieldList)
	{
		int ret = 0;
		clearERR();
		this.insertReq.Reset();
		if(dtcServer == null)
		{
			return false;
		}
		if(checkKey(key) == false)
		{
			return false;
		}
		if (check(fieldList) == false) {
			return false;
		}
		if ( key_type == 1 || key_type == 2)
		{
			ret = insertReq.SetKey(Long.parseLong(key));
		} 
		else if ( this.key_type == 4 || this.key_type == 5) 
		{
			ret = insertReq.SetKey(key, key.length());
		} 
		else 
		{
			this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
			this.errMsg  = "insert request failed: bad keyType!";
			return false;
		}
		if(0 != ret)
		{
			this.errCode = ret;
			this.errMsg  = "##dtc## insert setkey fail!";
			return false;
		}
		for(Iterator<Field> iter = fieldList.iterator(); iter.hasNext(); )
		{
			Field field = iter.next();
			switch(field.getFieldType())
			{
			case 1:
				ret = insertReq.Set(field.getFieldname(), Integer.parseInt(field.getFieldvalue()));
				break;
			case 2:
				ret = insertReq.Set(field.getFieldname(), Integer.parseInt(field.getFieldvalue()));
				break;
			case 3:
				ret = insertReq.Set(field.getFieldname(), Double.parseDouble(field.getFieldvalue()));
				break;
			case 4:
				ret = insertReq.Set(field.getFieldname(), field.getFieldvalue());
				break;
			case 5:
				ret = insertReq.Set(field.getFieldname(), field.getFieldvalue(), (field.getFieldvalue()).length());
				break;
			default:
				this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
				this.errMsg  = "##dtc## insert invalid feildType when set fields!";
				return false;
			}// end switch
		}// end for
		
		// 调用dtc api 执行插入操作
		Result rst = new Result();
		ret = insertReq.Execute(rst);
		if (0 != ret) 
		{
			this.dtcCode = rst.ResultCode();
			this.errCode = ret;
			this.errMsg = rst.ErrorMessage();
			this.errFrom = rst.ErrorFrom();		
			rst.DestroyResult();
			rst = null;
			return false;
		}
		this.affectRow = rst.AffectedRows();
		rst.DestroyResult();
		rst = null;
		return true;
	}

	/*
	 * @describe 检测数据是否合法
	 * @param fieldlist   List<Field>类型，包括所有的字段信息
	 */
	public boolean check(List<Field> fieldlist)
	{
		clearERR();
		for(Iterator<Field> iter = fieldlist.iterator(); iter.hasNext(); ) 
		{
			int i = 0;
			Field fd = iter.next();	
			if (fd == null) 
			{
				continue;
			}
			/* 获取该字段在DTCConfig.fieldcfg 数组中的 index */
			i = GetFlag(fd.getFieldname());
			if(i < 0)
			{
				this.errCode = DTCErrorCode.E_INVALID_FIELDNAME;
				this.errMsg = "invalid fieldname----can't find this field in dtcconfig! fieldname: " + fd.getFieldname();
				return false;
			}
			switch (fd.getFieldType())
			{
				case 1:
				case 2:
				case 3:
					int value = Integer.parseInt(fd.getFieldvalue());
					if (value < Long.parseLong(DTCConfig.fieldcfg[i][2])
							|| value > Long.parseLong(DTCConfig.fieldcfg[i][3])) 
					{
						this.errCode = DTCErrorCode.E_INVALID_FIELD_VALUE;
						this.errMsg  = "##dtc## int or float field error fieldvalue: " + value;
						return false;
					} 
					break;
				case 4:
					int len = (fd.getFieldvalue()).length();
					if (len < Long.parseLong(DTCConfig.fieldcfg[i][2])
							|| len > Long.parseLong(DTCConfig.fieldcfg[i][3])) 
					{
						this.errCode = DTCErrorCode.E_INVALID_FIELD_VALUE;
						this.errMsg  = "##dtc## string field error fieldvalue: " + fd.getFieldvalue() + "fieldlength: " + len;
						return false;
					} 
					break;
				case 5:
					int len1 = (fd.getFieldvalue()).length();
					if (len1 < Long.parseLong(DTCConfig.fieldcfg[i][2])
							|| len1 > Long.parseLong(DTCConfig.fieldcfg[i][3])) 
					{
						this.errCode = DTCErrorCode.E_INVALID_FIELD_VALUE;
						this.errMsg  = "##dtc## binary field error fieldvalue: " + fd.getFieldvalue() + "fieldlength: " + len1;
						return false;
					} 
					break;
				default:
					this.errCode = DTCErrorCode.E_INVALID_FIELD_TYPE;
					this.errMsg  = "##dtc## invalid fieldtype: " + fd.getFieldType();
					return false;
			}// switch
		}//for
		return true;
	}

	/*
	 * @describe 返回影响条数
	 * @return int AffectedRows
	 */
	public int  getAffectRows()
	{
		return this.affectRow;
	}
	
	/*
	 * @describe 检测数据是否合法
	 *
	 * @param key    表的主键
	 */
	public boolean checkKey(String key)
	{
		clearERR();		
		switch(this.key_type){
			case 1:
			case 2:
				long i = Long.parseLong(key);
				if(i < Long.parseLong(DTCConfig.fieldcfg[keyIndex][2]) 
						|| i > Long.parseLong(DTCConfig.fieldcfg[keyIndex][3]))
				{
					this.errCode = DTCErrorCode.E_INVALID_FIELD_VALUE;
					this.errMsg  = "##dtc## key error";
					return false;
				}
				break;
			case 4:
			case 5:
				int len = key.length();
				if (len < Long.parseLong(DTCConfig.fieldcfg[keyIndex][2])
						|| len > Long.parseLong(DTCConfig.fieldcfg[keyIndex][3])) 
				{
					this.errCode = DTCErrorCode.E_INVALID_FIELD_VALUE;
					this.errMsg  = "##dtc## key error";
					return false;
				} 
				break;
			default:
				this.errCode = DTCErrorCode.E_INVALID_KEY_TYPE;
				this.errMsg  = "##dtc## invalid keytype: " + this.key_type;
				return false;
		}
		return true;
	}

	/*
	 * 清除错误标识，在每个函数调用前调用
	 */
	private void clearERR()
	{
		errCode = 0;
		errMsg  = "";
		errFrom = "";
		dtcCode = 0;
	}
	
	/*
	 * 获取该字段在DTCConfig.fieldcfg 数组中的 index 
	 */
	private int GetFlag(String fieldName)
	{
		int flag = 0;
		int i=0;
		for(; i<DTCConfig.fieldcfg.length; i++)
		{
			if(DTCConfig.fieldcfg[i][0] == fieldName)
			{
				flag = i;
				break;
			}
		}
		if(i >= DTCConfig.fieldcfg.length) 		
			flag =-1;
		return flag;
	}
}//End Of file
