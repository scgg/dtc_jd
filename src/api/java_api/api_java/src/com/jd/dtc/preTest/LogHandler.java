package com.jd.dtc.preTest;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.FileHandler;
import java.util.logging.Formatter;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

public class LogHandler extends Formatter{
	private static Logger instance = null;

	@Override
	public String format(LogRecord record) {
		SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		return "["+df.format(new Date())+"]"+",level:"+record.getLevel()+",msg:"+record.getMessage()+"\n";
	}
	
	
	public static String getLogFileName(){
		StringBuffer logPath = new StringBuffer();
        //logPath.append(System.getProperty("user.home"));
        logPath.append("../log");
        File file = new File(logPath.toString());
        if (!file.exists())
            file.mkdir();
        
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        logPath.append("/java_test_"+sdf.format(new Date())+".log");
        
        return logPath.toString();
	}
	
	public static Logger initLog(String name){
		if( instance == null )
		{
			Logger logger = Logger.getLogger(name);
			if(PressureTest.level.equals("0") )
				logger.setLevel(Level.OFF);
			else if(PressureTest.level.equals("1") )
				logger.setLevel(Level.SEVERE);
			else if(PressureTest.level.equals("2") )
				logger.setLevel(Level.WARNING);
			else if(PressureTest.level.equals("3") )
				logger.setLevel(Level.INFO);
			else if(PressureTest.level.equals("4") )
				logger.setLevel(Level.CONFIG);
			else if(PressureTest.level.equals("5") )
				logger.setLevel(Level.FINE);
			else if(PressureTest.level.equals("6") )
				logger.setLevel(Level.FINER);
			else if(PressureTest.level.equals("7") )
				logger.setLevel(Level.FINEST);		
			
			try {
				FileHandler fileHandler = new FileHandler(getLogFileName(),true);
				if(PressureTest.level.equals("0") )
					fileHandler.setLevel(Level.OFF);
				else if(PressureTest.level.equals("1") )
					fileHandler.setLevel(Level.SEVERE);
				else if(PressureTest.level.equals("2") )
					fileHandler.setLevel(Level.WARNING);
				else if(PressureTest.level.equals("3") )
					fileHandler.setLevel(Level.INFO);
				else if(PressureTest.level.equals("4") )
					fileHandler.setLevel(Level.CONFIG);
				else if(PressureTest.level.equals("5") )
					fileHandler.setLevel(Level.FINE);
				else if(PressureTest.level.equals("6") )
					fileHandler.setLevel(Level.FINER);
				else if(PressureTest.level.equals("7") )
					fileHandler.setLevel(Level.FINEST);					
				
				fileHandler.setFormatter(new LogHandler());
				logger.addHandler(fileHandler);
			} catch (SecurityException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			instance = logger;
			
		}
		return instance;
	}

}
