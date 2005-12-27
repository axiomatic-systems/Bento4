package com.axiosys.bento4.metadata;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.axiosys.bento4.Atom;

public class StringAtom extends Atom {
    String value;
    
    public StringAtom(int type, int size, RandomAccessFile source) throws IOException {
        super(type, size, false);
        
        byte[] str = new byte[size-HEADER_SIZE];
        source.read(str);
        value = new String(str, "UTF-8");
    }
    
    public String getValue() { return value; }

    protected void writeFields(DataOutputStream stream) throws IOException {
        // not implemented yet
        throw new RuntimeException("not implemented yet");        
    }

    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n" + indentation + " string = " + value);
        return result.toString();  
    }
}
