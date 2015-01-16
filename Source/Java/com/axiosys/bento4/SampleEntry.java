/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: SampleEntry.java 196 2008-10-14 22:59:31Z bok $
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
****************************************************************/

package com.axiosys.bento4;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

public class SampleEntry extends ContainerAtom {
    private int dataReferenceIndex;
    
    SampleEntry(int format, int size, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(format, size, false, source);
        
        // read the fields before the children atoms
        int fieldsSize = getFieldsSize();
        readFields(source);

        // read children atoms (ex: esds and maybe others)
        readChildren(atomFactory, source, size-HEADER_SIZE-fieldsSize);
    }
    
    public void write(DataOutputStream stream) throws IOException {
        // write the header
        writeHeader(stream);

        // write the fields
        writeFields(stream);

        // write the children atoms
        writeChildren(stream);
    }
    
    protected int getFieldsSize() {
        return 8;
    }
    
    protected void readFields(RandomAccessFile source) throws IOException {
        source.skipBytes(6);
        dataReferenceIndex = source.readUnsignedShort();    
    }
    
    protected void writeFields(DataOutputStream stream) throws IOException {
        byte[] reserved = new byte[] { 0,0,0,0,0,0 };
        stream.write(reserved);
        stream.writeShort(dataReferenceIndex);
    }
 }
