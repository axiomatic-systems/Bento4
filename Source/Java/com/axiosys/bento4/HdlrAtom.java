/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: HdlrAtom.java 196 2008-10-14 22:59:31Z bok $
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

public class HdlrAtom extends Atom {
    private int    handlerType;
    private String handlerName;
    
    public HdlrAtom(int size, RandomAccessFile source) throws IOException {
        super(TYPE_HDLR, size, true, source);
        
        source.skipBytes(4);
        handlerType = source.readInt();
        source.skipBytes(12);
        
        // read the name unless it is empty
        int nameMaxSize = size-(FULL_HEADER_SIZE+20)-1;
        int nameSize = source.readByte();
        if (nameSize > nameMaxSize) nameSize = nameMaxSize;
        if (nameSize < 0) nameSize = 0;
        if (nameSize > 0) {
            byte[] name = new byte[nameSize];
            source.read(name);
            int nameChars = 0;
            while (nameChars < name.length && name[nameChars] != 0) nameChars++;
            handlerName = new String(name, 0, nameChars, "UTF-8");
        } 
    }

    public String getHandlerName() {
        return handlerName;
    }
    
    public int getHandlerType() {
        return handlerType;
    }
    
    protected void writeFields(DataOutputStream stream) throws IOException {
        // not implemented yet
        throw new RuntimeException("not implemented yet");
    }

    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n" + indentation + " handler_type      = " + Atom.typeString(handlerType));
        result.append("\n" + indentation + " handler_name      = " + handlerName);
        
        return result.toString();  
    }
}
