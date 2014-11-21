package com.jd.dtc.preTest;

import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.text.SimpleDateFormat;

import com.jd.dtc.api.DtcConsts;
import com.jd.dtc.api.Request;
import com.jd.dtc.api.Result;
import com.jd.dtc.api.Server;
import com.jd.dtc.example.TestWorker;
import com.jd.dtc.preTest.PressureTest.MyInfo;
import com.jd.dtc.template.Field;
import com.jd.dtc.template.It_dtc_exampleDTC;

import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.io.File;

public class PressureWorkThread extends Thread{

	private int count = 0;
	private It_dtc_exampleDTC dtc = null;
	private int optType;
	private int index = 0;
	
	public PressureWorkThread(int index, int count, int opt) {
		super();
		this.index = index;
		this.count = count;
		this.optType = opt;
		PressureTest.resultInfo[index] = new MyInfo();
	}
	public void get(String key){
		Logger logger = LogHandler.initLog(PressureWorkThread.class.getName());
		String value = PressureTest.key_value;
		if( value.equals("-1") )
		{
			long v = index*(PressureTest.request_cnt/PressureTest.thread_cnt)+Integer.parseInt(key)+100000000;			
			value = ""+v;
		}
		
		long begin = System.currentTimeMillis();
		List<Field> resultSet = dtc.get(value, null);
		long end = System.currentTimeMillis();
		
		if(resultSet == null )
		{
			logger.warning("thread index: "+index+", key:"+Integer.parseInt(value)+", dtc get fail,errCode: "+dtc.errorCode
					+ ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}
		if( dtc.dtcRetCode != 0 )
		{
			logger.warning("thread index: "+index+", key: "+Integer.parseInt(value)+", dtc retCode: "+dtc.dtcRetCode);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}	
		for(Iterator<Field> iter = resultSet.iterator(); iter.hasNext(); )
		{
			Field resultField = iter.next();
			logger.info("thread index: "+index+", key: "+Integer.parseInt(value)+", during: "+(end-begin) +
					"get fieldname: " + resultField.fieldname + ", fieldvalue: " + resultField.fieldvalue +
					", fieldtype: " + resultField.fieldType);
		}// end of for
		
		PressureTest.resultInfo[index].during= end-begin;
		PressureTest.setBegin(begin);
		PressureTest.setEnd(end);
	}
	
	
	public void insert(String key){
		Logger logger = LogHandler.initLog(PressureWorkThread.class.getName());
		String value = PressureTest.key_value;
		if( value.equals("-1") )
		{
			long v = index*(PressureTest.request_cnt/PressureTest.thread_cnt)+Integer.parseInt(key)+100000000;
			value = ""+v;	
		}
		
		List<Field> requestSet = new ArrayList<Field>();
		Field f1 = new Field();
		f1.fieldname = "uid";
		f1.fieldType = 2;
		f1.fieldvalue = value;
	
		Field f2 = new Field();
		f2.fieldname = "name";
		f2.fieldType = 4;
		f2.fieldvalue = "test java api";
		
		Field f3 = new Field();
		f3.fieldname = "city";
		f3.fieldType = 4;
		f3.fieldvalue = "shanghai";	
		
		Field f4 = new Field();
		f4.fieldname = "descr";
		f4.fieldType = 5;
		f4.fieldvalue = "test test test test test test";
		
		Field f5 = new Field();
		f5.fieldname = "age";
		f5.fieldType = 2;
		f5.fieldvalue = "23";
		
		Field f6 = new Field();
		f6.fieldname = "salary";
		f6.fieldType = 2;
		f6.fieldvalue = "8000";
		
		requestSet.add(f1);
		requestSet.add(f2);
		requestSet.add(f3);
		requestSet.add(f4);
		requestSet.add(f5);
		requestSet.add(f6);

		long begin = System.currentTimeMillis();
		boolean ret = dtc.insert(value, requestSet);
		long end = System.currentTimeMillis();
		
		if(ret == false )
		{
			logger.warning("thread index: "+index+", key:"+Integer.parseInt(value)+", dtc insert fail,errCode: "+dtc.errorCode
					+ ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}
		if( dtc.dtcRetCode != 0 )
		{
			logger.warning("thread index: "+index+", key:"+Integer.parseInt(value)+", dtc retCode: "+dtc.dtcRetCode);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}
		
		logger.info("thread index: "+index+", key: "+Integer.parseInt(value)+", during: "+(end-begin));
		PressureTest.resultInfo[index].during= end-begin;
		PressureTest.setBegin(begin);
		PressureTest.setEnd(end);
	}

	
	public void delete(String key){
		Logger logger = LogHandler.initLog(PressureWorkThread.class.getName());
		String value = PressureTest.key_value;
		if( value.equals("-1") )
		{
			long v = index*(PressureTest.request_cnt/PressureTest.thread_cnt)+Integer.parseInt(key)+100000000;
			value = ""+v;
		}
		long begin = System.currentTimeMillis();
		boolean ret = dtc.delete(value);
		long end = System.currentTimeMillis();
		if( ret == false )
		{
			logger.warning("thread index: "+index+", key: "+Integer.parseInt(value)+", delete ret: " +ret + 
					",errCode: " + dtc.errorCode + ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}
		if(dtc.dtcRetCode != 0 )
		{
			logger.warning("thread index: "+index+", key: "+Integer.parseInt(value)+", delete ret: " +ret + ", dtcRetCode: "
					+ dtc.dtcRetCode +", errCode: " + dtc.errorCode + ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}	
		logger.info("thread index: "+index+", key: "+Integer.parseInt(value)+", during: "+(end-begin));
		PressureTest.resultInfo[index].during = end-begin;
		PressureTest.setBegin(begin);
		PressureTest.setEnd(end);
	}
	
	public void update(String key){
		Logger logger = LogHandler.initLog(PressureWorkThread.class.getName());
		String value = PressureTest.key_value;
		if( value.equals("-1") )
		{
			long v = index*(PressureTest.request_cnt/PressureTest.thread_cnt)+Integer.parseInt(key)+100000000;
			value = ""+v;
		}
		Field f1 = new Field();
		f1.fieldname = "uid";
		f1.fieldType = 2;
		f1.fieldvalue = value;
		
		Field f2 = new Field();
		f2.fieldname = "name";
		f2.fieldType = 4;
		f2.fieldvalue = "test java api2";
		
		Field f3 = new Field();
		f3.fieldname = "city";
		f3.fieldType = 4;
		f3.fieldvalue = "shanghai2";
		
		Field f4 = new Field();
		f4.fieldname = "descr";
		f4.fieldType = 5;
		f4.fieldvalue = "test test test test test test2";
		
		Field f5 = new Field();
		f5.fieldname = "age";
		f5.fieldType = 2;
		f5.fieldvalue = "23";
		
		Field f6 = new Field();
		f6.fieldname = "salary";
		f6.fieldType = 2;
		f6.fieldvalue = "8000";
		
		List<Field> requestSet = new ArrayList<Field>();
		requestSet.add(f1);
		requestSet.add(f2);
		requestSet.add(f3);
		requestSet.add(f4);
		requestSet.add(f5);
		requestSet.add(f6);
		
		long begin = System.currentTimeMillis();
		boolean ret = dtc.update(value, requestSet);
		long end = System.currentTimeMillis();
		if( ret == false )
		{
			logger.warning("thread index: "+index+", key:"+Integer.parseInt(value)+", update ret: " +ret + 
					", errCode: " + dtc.errorCode + ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}
		if(dtc.dtcRetCode != 0 )
		{
			logger.warning("thread index: "+index+", key: "+Integer.parseInt(value)+", update ret: " +ret + ", dtcRetCode: "
					+ dtc.dtcRetCode +", errCode: " + dtc.errorCode + ", errMsg: " + dtc.errorMsg + ", errFrom: " + dtc.errorFrom);
			PressureTest.resultInfo[index].bFailed = true;
			return;
		}	
		logger.info("thread index: "+index+", key: "+Integer.parseInt(value)+", during: "+(end-begin));
		PressureTest.resultInfo[index].during = end-begin;
		PressureTest.setBegin(begin);
		PressureTest.setEnd(end);
	}

	public void replace(int i){
		
	}
	
	
	@Override
	public void run() {
		It_dtc_exampleDTC dtc = new It_dtc_exampleDTC();
		this.dtc = dtc;
		int i = 0;
		while (i < count) {
			switch(optType){
			case DtcConsts.RequestGet:
				get(Integer.toString(i));
				i++;
				break;
			case DtcConsts.RequestInsert:
				insert(Integer.toString(i));
				i++;
				break;
			case DtcConsts.RequestDelete:
				delete(Integer.toString(i));
				i++;
				break;
			case DtcConsts.RequestUpdate:
				update(Integer.toString(i));
				i++;
				break;
			case DtcConsts.RequestReplace:
				replace(i++);
				break;
			}
		}

		//System.out.println("thread:"+index);
		PressureTest.resultInfo[index].bFinished = true;
		dtc.DestroyRequest();
		dtc.DestroyServer();

	}	

}
