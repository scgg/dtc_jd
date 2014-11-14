package com.jd.dtc.preTest;

import com.jd.dtc.api.Request;
import com.jd.dtc.api.Result;
import com.jd.dtc.api.Server;

public class EasyTest {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		System.out.println(System.getProperty("java.library.path"));
		Server oServer = new Server(0);
		oServer.IntKey();
		oServer.SetTableName("ttc_test");
		oServer.SetAddress("127.0.0.1", "10100");
		oServer.SetTimeout(5);
		oServer.SetAccessKey("0000010184d9cfc2f395ce883a41d7ffc1bbcf4e");
		
		Request oRequest = new Request(oServer, 4);
		int ret = oRequest.SetKey(100001);
		if( ret == 0 )
			ret = oRequest.Need("data");
		if(ret == 0 )
			ret = oRequest.Need("len");
		
		Result oResult = new Result();
		ret = oRequest.Execute(oResult);
		
		ret = oResult.ResultCode();
		int numRows = oResult.NumRows();
		ret = oResult.FetchRow();
		System.out.println("oRequest.Execute ret:"+ret+"\nrows:"+numRows+"\ndata:"+oResult.BinaryValue("data")+"\nlen:"+oResult.IntValue("len"));
		
		
		
		
//		String key = oServer.GetTableName();
//		
//		System.out.print("key:"+key);
		
//		System.out.print("main thread begin");
//		
//		Thread th = new Thread(new Runnable() {
//			
//			@Override
//			public void run() {
//				// TODO Auto-generated method stub
//				try {
//					System.out.print("thread begin");
//					Thread.sleep(20000);
//					System.out.print("thread end");
//				} catch (InterruptedException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//			}
//		});
//		
//		th.start();
//		
//		try {
//			th.join();
//		} catch (InterruptedException e) {
//			// TODO Auto-generated catch block
//			e.printStackTrace();
//		}
//		System.out.print("main thread exit");
	}

}
