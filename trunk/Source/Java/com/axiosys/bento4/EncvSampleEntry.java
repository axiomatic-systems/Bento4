package com.axiosys.bento4;

import java.io.IOException;
import java.io.RandomAccessFile;


public class EncvSampleEntry extends VideoSampleEntry {
    public EncvSampleEntry(int size, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(TYPE_ENCV, size, source, atomFactory);
    }
}
