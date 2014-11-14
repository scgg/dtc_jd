package com.jd.dtc.example;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Logger;
import java.util.logging.Level;

import com.jd.dtc.api.DtcConsts;
import com.jd.dtc.api.Request;
import com.jd.dtc.api.Result;
import com.jd.dtc.api.Server;

public class TestWorker {

	/**
	 * @param args
	 */
	public static class Info{
		public Info(){
			bFailed = false;
			during = 0;
			bFinished = false;
		}
		public boolean bFailed = false;
		public long during = 0;
		public boolean bFinished = false;
	}
	
	
	public static Info[] resultInfo = null;  
	
	public static String tablename = "";
	public static String ip = "";
	public static String port = "";
	public static String token = "";
	public static String level = "1";//off=0,server=1,warning=2,info=3,config=4,fine=5,finer=6,finest=7
	public static int key_type = 0;//1-int,2-binary,3-string
	public static String key_value = "";
	public static int opt_type = 0;//4-get,6-insert,8-delete
	public static int thread_cnt = 0;
	public static int request_cnt = 0;
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
			Info info = resultInfo[i];
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
			new WorkThread(i,per_cnt,opt_type).start();
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
		System.out.println("usage: -t tablename -i dtc_agent_ip -p dtc_agent_port -a access_token -l log level -k key_type -v key_value -o opt_type" +
				" -c thread cnt -n request cnt -h help");
		System.out.println("\t -t tablename:the table to test");
		System.out.println("\t -i dtc_agent_ip:dtc or agent ip");
		System.out.println("\t -p dtc_agent_port:dtc or agent port");
		System.out.println("\t -a access_token:dtc agent's access token");
		System.out.println("\t -l log level:off=0,server=1,warning=2,info=3,config=4,fine=5,finer=6,finest=7");
		System.out.println("\t -k key_type:the key type to test:1-int,2-binary,3-string");
		System.out.println("\t -v key_value:the key value to test,-1-(1*100000000+0,c*100000000+p)");
		System.out.println("\t -o opt_type:4-get,6-insert,8-delete");
		System.out.println("\t -c thread cnt:count of threads");
		System.out.println("\t -n request cnt");
		System.out.println("\t -h help");
	}
	
	public static int parseArgs(String[] args){
		if( args.length == 1 && args[0].equals("-h") )
		{
			System.out.println(args.length+args[0]);
			showUsage();
			return 0;
		}
		
		if( args[0].equals("-t") )
			tablename = args[1];
		else
		{
			System.out.println("need set table name");
			return 0;
		}
		
		if( args[2].equals("-i") )
			ip = args[3];
		else
		{
			System.out.println("need dtc or agent ip");
			return 0;
		}
		
		if( args[4].equals("-p") )
			port = args[5];
		else{
			System.out.println("need dtc or agent port");
			return 0;
		}
		
		if( args[6].equals("-a") )
			token = args[7];
		else{
			System.out.println("need access token");
			return 0;
		}
		
		if( args[8].equals("-l") )
			level = args[9];
		else{
			System.out.println("need set log level");
			return 0;
		}
		
		if(args[10].equals("-k") )
			key_type = Integer.parseInt(args[11]);
		else{
			System.out.println("need set key type");
			return 0;
		}
		
		if(args[12].equals("-v") )
			key_value = args[13];
		else{
			System.out.println("need set key value");
			return 0;
		}
		
		if( args[14].equals("-o") )
			opt_type = Integer.parseInt(args[15]);
		else{
			System.out.println("need set opt type");
			return 0;
		}
		
		if( args[16].equals("-c") )
			thread_cnt = Integer.parseInt(args[17]);
		else{
			System.out.println("need set thread cnt");
			return 0;
		}
		
		if( args[18].equals("-n") )
			request_cnt = Integer.parseInt(args[19]);
		else{
			System.out.println("need set request cnt");
			return 0;
		}
		
		System.out.println("java -jar dtc_api.jar -t "+tablename + " -i " + ip + " -p " + port
				+" -a " + token + " -l " + level + " -k " + key_type + " -v " + key_value + " -o " + opt_type
				+" -c " + thread_cnt + " -n " + request_cnt);
		
		return 1;
	}
	
	public static void main(String[] args) throws Exception{
		
//		Server oServer = new Server();
//		oServer.IntKey();
//		oServer.SetTableName("test_dtc");
//		oServer.SetAddress("10.42.0.74", "10100");
//		oServer.SetAccessKey("0000010284d9cfc2f395ce883a41d7ffc1bbcf4e");
//		oServer.SetTableName("ttc_test");
//	    oServer.SetAddress("127.0.0.1", "10100");
//	    oServer.SetAccessKey("0000010184d9cfc2f395ce883a41d7ffc1bbcf4e");
//		oServer.SetTimeout(5);
        System.out.println(System.getProperty("java.library.path"));

		if(parseArgs(args) == 0)
			return;
		
		Logger logger = LogHandler.initLog(TestWorker.class.getName());
		int threadnum = thread_cnt;
		resultInfo = new Info[threadnum];
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
