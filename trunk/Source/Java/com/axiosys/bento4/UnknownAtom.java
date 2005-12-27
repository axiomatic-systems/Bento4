package com.axiosys.bento4;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

public class UnknownAtom extends Atom {

    private final RandomAccessFile source;
    private final long             sourceOffset;
    
    public UnknownAtom(int type, int size, RandomAccessFile source, long sourceOffset) {
        super(type, size, false);
        this.source        = source;
        this.sourceOffset = sourceOffset;
    }
    
    public UnknownAtom(int type, int size, RandomAccessFile source) throws IOException {
        this(type, size, source, source.getFilePointer());
    }
    
    public RandomAccessFile getSource()       { return source; }
    public long             getSourceOffset() { return sourceOffset; }

    protected void writeFields(DataOutputStream stream) throws IOException {
        // position into the source
        source.seek(sourceOffset);
        
        // read/write using a buffer
        int read;
        byte[] buffer = new byte[4096];        
        while ((read = source.read(buffer)) != -1) {
            stream.write(buffer, 0, read);
        }
    }
}
