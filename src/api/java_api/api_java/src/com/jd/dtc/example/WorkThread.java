package com.jd.dtc.example;

import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.text.SimpleDateFormat;

import com.jd.dtc.api.DtcConsts;
import com.jd.dtc.api.Request;
import com.jd.dtc.api.Result;
import com.jd.dtc.api.Server;
import com.jd.dtc.example.TestWorker.Info;

import java.util.Date;
import java.io.File;

public class WorkThread extends Thread{

	private int count = 0;
	private Server oServer;
	private int optType;
	private int index = 0;
	
	
	public WorkThread(int index, int count, int opt) {
		super();
		this.index = index;
		this.count = count;
		this.optType = opt;
		TestWorker.resultInfo[index] = new Info();
	}

	
	
	
	public void get(int i){
		Logger logger = LogHandler.initLog(WorkThread.class.getName());
		Request oGetRequest = new Request(oServer, optType);
		String value = TestWorker.key_value;
		if( value.equals("-1") )
		{
			long v = index*(TestWorker.request_cnt/TestWorker.thread_cnt)+i+100000000;
			value = ""+v;
		}
		
			
		int ret = 0;
		switch(TestWorker.key_type)
		{
		case 1:
			ret = oGetRequest.SetKey(Long.parseLong(value));
			break;
		case 2:
		case 3:
			ret = oGetRequest.SetKey(value);
			break;
			
		}
		
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.SetKey false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		ret = oGetRequest.Need("data");
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Need data false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		
		ret = oGetRequest.Need("len");
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Need len false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		Result oResult = new Result();
		long begin = oGetRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		ret = oGetRequest.Execute(oResult);
		long end = oGetRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Execute false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			oGetRequest.DestroyRequest();
			return;
		}
		ret = oResult.ResultCode();
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Execute false,result.ResultCode:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			oGetRequest.DestroyRequest();
			return;
		}
		
		int numRows = oResult.NumRows();
		ret = oResult.FetchRow();
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Execute false,result.FetchRow:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			oGetRequest.DestroyRequest();
			return;
		}
		
		logger.info("thread index:"+index+",i:"+i+",during:"+(end-begin)+",oGetRequest.Execute ret:"+ret+",rows:"+numRows+",data:"+oResult.BinaryValue("data")+",len:"+oResult.IntValue("len"));
		TestWorker.resultInfo[index].during= end-begin;
		TestWorker.setBegin(begin);
		TestWorker.setEnd(end);
		oGetRequest.DestroyRequest();
		oGetRequest = null;
	}
	
	
	public void insert(int i){
		Logger logger = LogHandler.initLog(WorkThread.class.getName());
		Request oInsertRequest = new Request(oServer, optType);
		String value = TestWorker.key_value;
		if( value.equals("-1") )
		{
			long v = index*(TestWorker.request_cnt/TestWorker.thread_cnt)+i+100000000;
			value = ""+v;
		}
		
		int ret = 0;
		switch(TestWorker.key_type)
		{
		case 1:
			ret = oInsertRequest.SetKey(Long.parseLong(value));
			break;
		case 2:
		case 3:
			ret = oInsertRequest.SetKey(value);
			break;
			
		}
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oInsertRequest.SetKey false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		ret = oInsertRequest.Set("data","yyyyyyyyyyyyyyy");
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oInsertRequest.Set data false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		//logger.info("set len");
		ret = oInsertRequest.Set("len",9);
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oInsertRequest.Set len false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		Result oResult = new Result();
		long begin = oInsertRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		ret = oInsertRequest.Execute(oResult);
		long end = oInsertRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oInsertRequest.Execute false,ret:"+ret
					+",errmsg:"+oResult.ErrorMessage()+",from:"+oResult.ErrorFrom());
			TestWorker.resultInfo[index].bFailed = true;
			oInsertRequest.DestroyRequest();
			return;
		}
		ret = oResult.ResultCode();
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oInsertRequest.Execute false,result.ResultCode:"+ret
					+",errmsg:"+oResult.ErrorMessage()+",from:"+oResult.ErrorFrom());
			TestWorker.resultInfo[index].bFailed = true;
			oInsertRequest.DestroyRequest();
			return;
		}
		
		int numRows = oResult.NumRows();
		logger.info("thread index:"+index+",i:"+i+",during:"+(end-begin)+",oInsertRequest.Execute ret:"+ret+",errmsg:"+oResult.ErrorMessage()+",rows:"+numRows);
		TestWorker.resultInfo[index].during = end-begin;
		TestWorker.setBegin(begin);
		TestWorker.setEnd(end);
		oInsertRequest.DestroyRequest();
	}
	
	public void delete(int i){
		Logger logger = LogHandler.initLog(WorkThread.class.getName());
		Request oDeleteRequest = new Request(oServer, optType);
		String value = TestWorker.key_value;
		if( value.equals("-1") )
		{
			long v = index*(TestWorker.request_cnt/TestWorker.thread_cnt)+i+100000000;
			value = ""+v;
		}
		
		int ret = 0;
		switch(TestWorker.key_type)
		{
		case 1:
			ret = oDeleteRequest.SetKey(Long.parseLong(value));
			break;
		case 2:
		case 3:
			ret = oDeleteRequest.SetKey(value);
			break;
			
		}
		
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oDeleteRequest.SetKey false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			return;
		}
		
		Result oResult = new Result();
		long begin = oDeleteRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		ret = oDeleteRequest.Execute(oResult);
		long end = oDeleteRequest.GetCurrentUSeconds();//System.currentTimeMillis();
		
		if(ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Execute false,ret:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			oDeleteRequest.DestroyRequest();
			return;
		}
		ret = oResult.ResultCode();
		if( ret != 0 )
		{
			logger.warning("thread index:"+index+",i:"+i+",oGetRequest.Execute false,result.ResultCode:"+ret);
			TestWorker.resultInfo[index].bFailed = true;
			oDeleteRequest.DestroyRequest();
			return;
		}
		
		int numRows = oResult.NumRows();
		logger.info("thread index:"+index+",i:"+i+",during:"+(end-begin)+",oInsertRequest.Execute ret:"+ret+",rows:"+numRows);
		TestWorker.resultInfo[index].during = end-begin;
		TestWorker.setBegin(begin);
		TestWorker.setEnd(end);
		oDeleteRequest.DestroyRequest();
		
	}
	
	public void update(int i){
		
	}

	public void replace(int i){
		
	}
	
	
	@Override
	public void run() {
		//System.out.println("thread:"+index);
		Server oServer = new Server(index);
		//TestWorker.setBegin(oServer.GetCurrentUSeconds());
		switch(TestWorker.key_type)
		{
		case 1:
			oServer.IntKey();
			break;
		case 2:
			oServer.BinaryKey();
			break;
		case 3:
			oServer.StringKey();
			break;
		}
		
		oServer.SetTableName(TestWorker.tablename);
		oServer.SetAddress(TestWorker.ip, TestWorker.port);
		oServer.SetAccessKey(TestWorker.token);
		this.oServer = oServer;
		int i = 0;
		while (i < count) {
			switch(optType){
			case DtcConsts.RequestGet:
				get(i++);
				break;
			case DtcConsts.RequestInsert:
				insert(i++);
				break;
			case DtcConsts.RequestDelete:
				delete(i++);
				break;
			case DtcConsts.RequestUpdate:
				update(i++);
				break;
			case DtcConsts.RequestReplace:
				replace(i++);
				break;
			}
		}

		//System.out.println("thread:"+index);
		TestWorker.resultInfo[index].bFinished = true;
		//TestWorker.setEnd(oServer.GetCurrentUSeconds());
		//oServer.DestroyServer();
		//oServer = null;

	}
	

}
