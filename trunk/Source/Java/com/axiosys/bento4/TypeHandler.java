package com.axiosys.bento4;

import java.io.IOException;
import java.io.RandomAccessFile;

public interface TypeHandler {
    public Atom createAtom(int type, int size, RandomAccessFile file, int context) throws IOException, InvalidFormatException;
}
