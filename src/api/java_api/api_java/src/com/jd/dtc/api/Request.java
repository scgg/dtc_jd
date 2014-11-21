package com.jd.dtc.api;


public class Request {
	private long oServer = 0;
	private long oRequest = 0;
	
	public Request(Server server, int op){
		InitRequest(server,op);
	}
	public Request(){
		InitRequest(null,0);
	}

/*	@Override
	protected void finalize() throws Throwable {
		//DestroyRequest();
		//oRequest = -1;
		super.finalize();
	}*/

	public long getServer() {
		return oServer;
	}

	public void setServer(long oServer) {
		this.oServer = oServer;
	}

	public long getRequest() {
		//System.out.println("getRequest:"+Long.toHexString(oRequest));
		return oRequest;
	}

	public void setRequest(long oRequest) {
		//System.out.println("setRequest:"+Long.toHexString(oRequest));
		this.oRequest = oRequest;
	}
	
	public int SetKey(String key){
		return SetKey(key,key.length());
	}
	
	public Result Execute(){
		Result oResult = null;
		long result_addr = JExecute();
		if( result_addr < 0 )
			return oResult;
		oResult = new Result();
		oResult.setResult(result_addr);
		
		return oResult;
	}
	public Result Execute(long k){
		Result oResult = null;
		long result_addr = JExecute(k);
		if( result_addr < 0 )
			return oResult;
		oResult = new Result();
		oResult.setResult(result_addr);
		
		return oResult;
	}
	public Result Execute(String k){
		Result oResult = null;
		long result_addr = JExecute(k);
		if( result_addr < 0 )
			return oResult;
		oResult = new Result();
		oResult.setResult(result_addr);
		
		return oResult;
	}
	public Result Execute(String k, int l){
		Result oResult = null;
		long result_addr = JExecute(k, l);
		if( result_addr < 0 )
			return oResult;
		oResult = new Result();
		oResult.setResult(result_addr);
		
		return oResult;
	}

	public native void InitRequest(Server oServer, int op);
	public native void DestroyRequest();
	public native void Reset();
	public native void Reset(int op);
	public native int AttachServer(Server oServer);
	public native void NoCache();
	public native void NoNextServer();
	public native int Need(String name);
	public native int Need(String name, int vid);
	public native int FieldType(String name);
	public native void Limit(int st, int cnt);
	public native void UnsetKey();
	public native int SetKey(long key);
	
	public native int SetKey(String key, int len);
	public native int AddKeyValue(String name, long v);
	public native int AddKeyValue(String name, String v);
	public native int AddKeyValue(String name, String v, long len);
	public native int SetCacheID(long id);
	public native long JExecute();
	public native long JExecute(long k);
	public native long JExecute(String k);
	public native long JExecute(String k, int l);
	public native int Execute(Result s);
	public native int Execute(Result s, long k);
	public native int Execute(Result s, String k);
	public native int Execute(Result s, String k, int l);
	public native String ErrorMessage();
	
	public native int EQ(String n, long v);
	public native int EQ(String n, String v);
	public native int EQ(String n, String v, long l);
	
	public native int NE(String n, long v);
	public native int NE(String n, String v);
	public native int NE(String n, String v, long l);
	
	public native int GT(String n, long v);
	public native int GE(String n, long v);
	public native int LE(String n, long v);
	public native int LT(String n, long v);
	
	public native int OR(String n, long v);
	
	public native int Add(String n, long v);
	public native int Add(String n, double v);
	
	public native int Sub(String n, double v);
	public native int Sub(String n, long v);
	
	public native int Set(String n, double v);
	public native int Set(String n, String v);
	public native int Set(String n, String v, long l);
	public native int Set(String n, long v);
	public native int Set(String n, int v);
	
	public native int CompressSet(String n, String v, long len);
	public native int CompressSetForce(String n, String v, long len);
	public native int SetMultiBits(String n, int o, int s, int v);
	public native long GetCurrentUSeconds();
}

