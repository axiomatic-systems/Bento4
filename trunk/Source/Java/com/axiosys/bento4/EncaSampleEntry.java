package com.axiosys.bento4;

import java.io.IOException;
import java.io.RandomAccessFile;


public class EncaSampleEntry extends AudioSampleEntry {
    public EncaSampleEntry(int size, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(TYPE_ENCA, size, source, atomFactory);
    }
}
