/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: OhdrAtom.java 196 2008-10-14 22:59:31Z bok $
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
import com.axiosys.bento4.AtomFactory;
import com.axiosys.bento4.ContainerAtom;
import com.axiosys.bento4.InvalidFormatException;

public class OhdrAtom extends ContainerAtom {
    private int     encryptionMethod;
    private int     paddingScheme;
    private long    plaintextLength;
    private String  contentId;
    private String  rightsIssuerUrl;
    private String  textualHeaders;
    
    public OhdrAtom(int size, RandomAccessFile source, AtomFactory factory) throws IOException, InvalidFormatException {
        super(Atom.TYPE_OHDR, size, true, source);
        
        // read the fields before the children
        encryptionMethod = source.readByte();
        paddingScheme    = source.readByte();
        plaintextLength  = source.readLong();
        
        byte[] contentIdBytes     = new byte[source.readShort()];
        byte[] rightsIssuerBytes  = new byte[source.readShort()];
        byte[] textualHeaderBytes = new byte[source.readShort()];
        source.readFully(contentIdBytes);
        source.readFully(rightsIssuerBytes);
        source.readFully(textualHeaderBytes);
        contentId       = new String(contentIdBytes);
        rightsIssuerUrl = new String(rightsIssuerBytes);
        textualHeaders  = new String(textualHeaderBytes);
        
        // now read the children
        int fieldsSize = 2 + 8 + 6 + contentIdBytes.length + rightsIssuerBytes.length + textualHeaderBytes.length;  
        readChildren(factory, source, size-FULL_HEADER_SIZE-fieldsSize);        
    }

    protected void writeFields(DataOutputStream stream) throws IOException {
        throw new RuntimeException("Not implemented yet");

    }
    
    public String toString(String indentation) {
        StringBuffer result = new StringBuffer(super.toString(indentation));
        result.append("\n" + indentation + " encryption_method = " + OmaDcf.encryptionMethodToString(encryptionMethod));
        result.append("\n" + indentation + " padding_scheme    = " + OmaDcf.paddingSchemeToString(paddingScheme));
        result.append("\n" + indentation + " plain_text_length = " + plaintextLength);
        result.append("\n" + indentation + " content_id        = " + contentId);
        result.append("\n" + indentation + " rights_issuer_url = " + rightsIssuerUrl);
        result.append("\n" + indentation + " textual_headers   = " + textualHeaders);
        return result.toString();          
        
    }

    public String getContentId() {
        return contentId;
    }

    public int getEncryptionMethod() {
        return encryptionMethod;
    }

    public int getPaddingScheme() {
        return paddingScheme;
    }

    public long getPlaintextLength() {
        return plaintextLength;
    }

    public String getRightsIssuerUrl() {
        return rightsIssuerUrl;
    }

    public String getTextualHeaders() {
        return textualHeaders;
    }

}
