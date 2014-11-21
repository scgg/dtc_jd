package com.jd.dtc.template;

import java.io.Serializable;

public class Field implements Serializable{

    private static final long serialVersionUID = 275871992933449236L;
    public String fieldname;
    public int fieldType;        // None=0, Signed=1, Unsigned=2, Float=3, String=4, Binary=5
    public String fieldvalue;

    public Field() {
    }

    public String getFieldname() {
        return fieldname;
    }

    public void setFieldname(String fieldname) {
        this.fieldname = fieldname;
    }

    public int getFieldType() {
        return fieldType;
    }

    public void setFieldType(int fieldType) {
        this.fieldType = fieldType;
    }

    public String getFieldvalue() {
        return fieldvalue;
    }

    public void setFieldvalue(String fieldvalue) {
        this.fieldvalue = fieldvalue;
    }



}
