/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: IkmsAtom.java 196 2008-10-14 22:59:31Z bok $
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

package com.axiosys.bento4.ismacryp;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.axiosys.bento4.Atom;

public class IkmsAtom extends Atom {
    private final String kmsUri;
    
    public IkmsAtom(int size, RandomAccessFile source) throws IOException {
        super(TYPE_IKMS, size, true, source);
        
        byte[] str = new byte[size-getHeaderSize()];
        source.read(str);
        int str_size = 0;
        while (str[str_size] != 0) str_size++;
        kmsUri = new String(str, 0, str_size, "UTF-8");
    }
    
    public String getKmsUri() { 
        return kmsUri;
    }
    
    protected void writeFields(DataOutputStream stream) throws IOException {
        byte[] bytes = kmsUri.getBytes("UTF-8");
        stream.write(bytes);
        int termination = size-getHeaderSize()-bytes.length;
        for (int i=0; i<termination; i++) {
            stream.writeByte(0);
        }
    }
    
    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n");
        result.append(indentation + "  kms_uri = " + kmsUri);
        return result.toString();
    }
}
