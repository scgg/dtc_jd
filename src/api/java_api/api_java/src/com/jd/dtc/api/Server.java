package com.jd.dtc.api;

import java.util.Iterator;
import java.util.Map;



public class Server {
	static{
		String OS = System.getProperty("os.name".toLowerCase());
		if(OS.contains("windows"))
			System.loadLibrary("_dtc_java_api");
		else
			System.load("/usr/lib/lib_dtc_java_api.so");
	}
	public Server(int id) {
		this.id = id;
		InitServer();
	}
	public Server( Server server ){
		oServer = CloneServer(server);
	}
/*	@Override
	protected void finalize() throws Throwable {
		//System.out.println("server finalize:"+id);
		//DestroyServer();
		super.finalize();
	}*/
	public native void InitServer();
	public native int SetTableName( String name);
	public native void DestroyServer();
	public native int SetAddress( String host, String port);
	public native void CloneTabDef( Server server );
	public native long CloneServer( Server server );
	//for compress
    public native void SetCompressLevel(int level);
    //get address and tablename set by user
	public native String GetAddress();
	public native String GetTableName();
    //get address and tablename set by ttc frame,for plugin only;
	public native String GetServerAddress();
	public native String GetServerTableName();
	public native int IntKey();
	public native int BinaryKey();
	public native int StringKey();
	public native int AddKey(String name, int type);
	public native int FieldType(String name);
	public native String ErrorMessage();
	public native void SetTimeout(int timeout);
	public native void SetMTimeout(int m_timeout);
	public native int Connect();
	public native void Close();
	public native int Ping();
	public native void AutoPing();
	public native void SetFD(int fd); // UNSUPPORTED API
	public native void SetAutoUpdateTab(boolean autoUpdate);
	public native void SetAutoReconnect(int autoReconnect);
	//public native int DecodePacket(Result &, const char *, int);
	public native int CheckPacketSize( String packet, int size);

	public native void SetAccessKey(String token);
	public native long GetCurrentUSeconds();
	
	
	private long oServer;
	private int id;
	
	public void setServer(long oServer )
	{
		this.oServer = oServer;
	}
	
	public long getServer()
	{
		return oServer;
	}
		
}
