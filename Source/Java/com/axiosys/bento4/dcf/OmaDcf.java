/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: OmaDcf.java 196 2008-10-14 22:59:31Z bok $
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

public class OmaDcf {
    // encryption
    public static final int ENCRYPTION_METHOD_NULL    = 0;
    public static final int ENCRYPTION_METHOD_AES_CBC = 1;
    public static final int ENCRYPTION_METHOD_AES_CTR = 2;
    
    // padding
    public static final int PADDING_SCHEME_NONE       = 0;
    public static final int PADDING_SCHEME_RFC_2630   = 1;
    
    public static String encryptionMethodToString(int method) {
        switch (method) {
        case ENCRYPTION_METHOD_NULL:
            return "Null";
        case ENCRYPTION_METHOD_AES_CBC:
            return "AES CBC";
        case ENCRYPTION_METHOD_AES_CTR:
            return "AES CTR";
        default:
            return "Unknown";
        }
    }
    
    public static String paddingSchemeToString(int scheme) {
        switch (scheme) {
        case PADDING_SCHEME_NONE:
            return "None";
        case PADDING_SCHEME_RFC_2630:
            return "RFC 2630";
        default:
            return "Unknown";
        }
    }
    

}
