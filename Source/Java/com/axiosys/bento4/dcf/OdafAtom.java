package com.axiosys.bento4.dcf;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.axiosys.bento4.Atom;

public class OdafAtom extends Atom {
    // members
    private boolean selectiveEncryption;
    private int     keyIndicatorLength;
    private int     ivLength;
    
    public OdafAtom(int size, RandomAccessFile source) throws IOException {
        super(Atom.TYPE_ODAF, size, true, source);
        byte s = source.readByte();
        selectiveEncryption = ((s&0x80) != 0);
        keyIndicatorLength = source.readByte();
        ivLength = source.readByte(); 
    }

    protected void writeFields(DataOutputStream stream) throws IOException {
        throw new RuntimeException("Not implemented yet");

    }

    public int getIvLength() {
        return ivLength;
    }

    public int getKeyIndicatorLength() {
        return keyIndicatorLength;
    }

    public boolean getSelectiveEncryption() {
        return selectiveEncryption;
    }
    
    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n" + indentation + " selective_encryption = " + selectiveEncryption);
        result.append("\n" + indentation + " key_indicator_length = " + keyIndicatorLength);
        result.append("\n" + indentation + " iv_ength             = " + ivLength);
        return result.toString();          
    }

}
