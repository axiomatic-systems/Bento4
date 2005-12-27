package com.axiosys.bento4.metadata;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.axiosys.bento4.Atom;

public class DataAtom extends Atom {
    public final static int  DATA_TYPE_BINARY             = 0;
    public final static int  DATA_TYPE_STRING_UTF8        = 1;
    public final static int  DATA_TYPE_STRING_UTF16       = 2;
    public final static int  DATA_TYPE_STRING_MAC_ENCODED = 3;
    public final static int  DATA_TYPE_JPEG               = 14;
    public final static int  DATA_TYPE_SIGNED_INT_BE      = 21; /* the size of the integer is derived from the container size */
    public final static int  DATA_TYPE_FLOAT32_BE         = 22;
    public final static int  DATA_TYPE_FLOAT64_BE         = 23;

    private int              dataType;
    private int              dataLang;
    private RandomAccessFile source;
    private long             sourceOffset;
    private int              payloadSize;
    
    public DataAtom(int size, RandomAccessFile source) throws IOException {
        super(MetaData.TYPE_DATA, size, false);

        if (size < Atom.HEADER_SIZE+8) return;

        dataType = source.readInt();
        dataLang = source.readInt();

        // the stream for the data is a substream of this source
        this.source = source;
        sourceOffset = source.getFilePointer();
        payloadSize  = size-Atom.HEADER_SIZE-8;
    }

    // accessors
    int getDataType() { return dataType; }
    int getDataLang() { return dataLang; }

    // methods
    String loadString() throws IOException {
        return new String(loadBytes(), "UTF-8");
    }
    
    byte[] loadBytes() throws IOException {
        source.seek(sourceOffset);
        byte[] str = new byte[payloadSize];
        source.read(str);
        
        return str;
    }
    
    long loadInteger() throws IOException {
        source.seek(sourceOffset);
        switch (payloadSize) {
            case 1:
                return source.readByte();
                
            case 2:
                return source.readShort();
                
            case 4:
                return source.readInt();
                
            case 8:
                return source.readLong();
                
            default:
                throw new IOException("invalid integer size");
        }
    }

    protected void writeFields(DataOutputStream stream) throws IOException {
        // not implemented yet
        throw new RuntimeException("not implemented yet");
    }
    
    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n" + indentation + " data_type = " + dataType);
        result.append("\n" + indentation + " data_lang = " + dataLang);
        String dataString;
        try {
            switch (dataType) {
                case DATA_TYPE_STRING_UTF8: dataString = loadString(); break;
                case DATA_TYPE_SIGNED_INT_BE: dataString = Integer.toString((int) loadInteger());  break;
                default: dataString = Integer.toString(payloadSize) + " bytes"; break;
            }
        } catch (Exception e) {
            dataString = "";
        }
        
        result.append("\n" + indentation + " data      = " + dataString);
        return result.toString();  
    }
}
