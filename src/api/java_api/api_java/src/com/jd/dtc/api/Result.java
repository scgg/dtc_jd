package com.jd.dtc.api;

public class Result {
	private long oResult;
	
	public Result(){
	}
	
	
	
/*	@Override
	protected void finalize() throws Throwable {
		Reset();
		super.finalize();
	}*/


	public native void Reset();
	public native void SetError(int err, String via, String msg);
	public native int FetchRow();
	public native int Rewind();
	public native int ResultCode();
	public native String ErrorMessage();
	public native String ErrorFrom();
	public native String ServerInfo();
	public native long BinlogID();
	public native long BinlogOffset();
	public native long MemSize();
	public native long DataSize();
	public native int NumRows();
	public native int TotalRows();
	public native long InsertID();
	public native int NumFields();
	public native String FieldName(int n);
	public native int FieldPresent(String name);
	public native int FieldType(int n);
	public native int AffectedRows();
	public native long IntKey();
	public native String StringKey();
	public native String BinaryKey();
	
	public native long IntValue(String name);
	public native long IntValue(int id);
	
	public native double FloatValue(String name);
	public native double FloatValue(int id);
	
	public native String StringValue(String name );
	public native String StringValue(int id);
	
	public native String BinaryValue(String name);
	public native String BinaryValue(int id);
	
	public native void DestroyResult();

	public long getResult() {
		return oResult;
	}

	public void setResult(long oResult) {
		this.oResult = oResult;
	}
	

}
