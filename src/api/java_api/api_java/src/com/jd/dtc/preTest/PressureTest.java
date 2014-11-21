package com.jd.dtc.preTest;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Logger;
import java.util.logging.Level;

import com.jd.dtc.example.LogHandler;
import com.jd.dtc.template.DTCt_dtc_example;

public class PressureTest {

	/**
	 * @param args
	 */
	public static class MyInfo{
		public MyInfo(){
			bFailed = false;
			during = 0;
			bFinished = false;
		}
		public boolean bFailed = false;
		public long during = 0;
		public boolean bFinished = false;
	}
	
	
	public static MyInfo[] resultInfo = null;  
	
	public static String level = "1";//off=0,server=1,warning=2,info=3,config=4,fine=5,finer=6,finest=7
	public static int opt_type = 0;//4-get,6-insert,8-delete
	
	public static int thread_cnt = 0;
	public static int request_cnt = 0;
	
	public static String key_value = "";
	public static long begin = 0;
	public static long end = 0;
	public static long maxDuring = 0;
	public static long minDuring = 10000;
	public static int failedCnt = 0;
	
	
	public static boolean isFinished(){
		for( int i = 0; i < thread_cnt; i++ )
		{
			if( !resultInfo[i].bFinished )
				return false;
		}
		
		failedCnt = 0;
		maxDuring = 0;
		minDuring = 10000;
		for( int i = 0; i < thread_cnt; i++ )
		{
			MyInfo info = resultInfo[i];
			if(info.bFailed)
				failedCnt++;
			if(maxDuring < info.during)
				maxDuring = info.during;
			if(minDuring > info.during )
				minDuring = info.during;
		}
		
		return true;
		
	}
	public static void TestOpt(){
		
		int count = thread_cnt;
		int per_cnt = request_cnt/count;
		int last_cnt = per_cnt + request_cnt%count;
		for( int i = 0; i < count; i++ )
		{
			//System.out.println("TestOpt:"+i);
			if( i == count-1 )
				per_cnt = last_cnt;
			new PressureWorkThread(i,per_cnt,opt_type).start();
		}
	}
	
	
	public static void setBegin(long beginTime){
		//System.out.println("begin:"+beginTime);
		if( begin == 0 )
			begin = beginTime;
	}
	
	public static void setEnd(long endTime){
		//System.out.println("end:"+endTime);
		if(end == 0 ||end < endTime)
			end = endTime;
	}
	
	public static void showUsage(){
		System.out.println("usage: -l log level -o opt_type -c thread cnt -n request cnt -h help");
		System.out.println("\t -l log level:off=0,server=1,warning=2,info=3,config=4,fine=5,finer=6,finest=7");
		System.out.println("\t -o opt_type:4-get, 6-insert, 7-update, 8-delete");
		System.out.println("\t -c thread cnt:count of threads");
		System.out.println("\t -n request cnt");
		System.out.println("\t -v key value");
		System.out.println("\t -h help");
	}
	
	public static int parseArgs(String[] args){
		if(args.length == 0)
		{
			System.out.println("need set options");
			showUsage();
			return 0;
		}
		else if( args.length == 1 && args[0].equals("-h") )
		{
			System.out.println(args.length+args[0]);
			showUsage();
			return 0;
		}	
		if( args[0].equals("-l") )
			level = args[1];
		else{
			System.out.println("need set log level");
			return 0;
		}	
		if( args[2].equals("-o") )
			opt_type = Integer.parseInt(args[3]);
		else{
			System.out.println("need set opt type");
			return 0;
		}	
		if( args[4].equals("-c") )
			thread_cnt = Integer.parseInt(args[5]);
		else{
			System.out.println("need set thread cnt");
			return 0;
		}
		if( args[6].equals("-n") )
			request_cnt = Integer.parseInt(args[7]);
		else{
			System.out.println("need set request cnt");
			return 0;
		}
		
		if( args[8].equals("-v") )
			key_value = args[9];
		else{
			System.out.println("need set key value");
			return 0;
		}
		
		System.out.println("java -jar dtc_api.jar" + " -l " + level + " -o " + opt_type
				+" -c " + thread_cnt + " -n " + request_cnt + " -v " + key_value);
		return 1;
	}
	
	public static void main(String[] args) throws Exception{
        System.out.println(System.getProperty("java.library.path"));
		if(parseArgs(args) == 0)
			return;
		Logger logger = LogHandler.initLog(PressureTest.class.getName());
		logger.info("level:" + level + ", opt_type: " + opt_type + ", thread_cnt: " + thread_cnt +
				", request_cnt: " + request_cnt);
		int threadnum = thread_cnt;
		resultInfo = new MyInfo[threadnum];
		TestOpt();
		while(!isFinished())
		{
			//System.out.println(thread_cnt);
			Thread.sleep(500);
			
		}
		System.out.println("\nthread cnt:"+threadnum+"\nrequestCnt:"+request_cnt+"\noptType:"+opt_type
				+"\nbegin:"+begin+"\nend:"+end+"\nduring:"+(end-begin)+"\ntimes:"+(request_cnt*1000000.0)/(end-begin)
				+"\nfailCnt:"+failedCnt+"\nmaxDuring:"+maxDuring+"\nminDuring:"+minDuring);
		
			
	}

}
