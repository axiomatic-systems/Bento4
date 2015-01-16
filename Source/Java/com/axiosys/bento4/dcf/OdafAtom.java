/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: OdafAtom.java 196 2008-10-14 22:59:31Z bok $
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
