/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: MetaDataTypeHandler.java 196 2008-10-14 22:59:31Z bok $
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

package com.axiosys.bento4.metadata;

import java.io.IOException;
import java.io.RandomAccessFile;

import com.axiosys.bento4.Atom;
import com.axiosys.bento4.AtomFactory;
import com.axiosys.bento4.ContainerAtom;
import com.axiosys.bento4.InvalidFormatException;
import com.axiosys.bento4.TypeHandler;

public class MetaDataTypeHandler implements TypeHandler {
    private AtomFactory atomFactory;

    private static boolean isMetaDataType(int type) {
        switch (type) {
            case MetaData.TYPE_dddd:
            case MetaData.TYPE_cNAM:
            case MetaData.TYPE_cART:
            case MetaData.TYPE_cCOM:
            case MetaData.TYPE_cWRT:
            case MetaData.TYPE_cALB:
            case MetaData.TYPE_cGEN:
            case MetaData.TYPE_cGRP:
            case MetaData.TYPE_cDAY:
            case MetaData.TYPE_cTOO:
            case MetaData.TYPE_cCMT:
            case MetaData.TYPE_CPRT:
            case MetaData.TYPE_TRKN:
            case MetaData.TYPE_DISK:
            case MetaData.TYPE_COVR:
            case MetaData.TYPE_DESC:
            case MetaData.TYPE_GNRE:
            case MetaData.TYPE_CPIL:
//            case MetaData.TYPE_TMPO:
//            case MetaData.TYPE_RTNG:
//            case MetaData.TYPE_apID:
//            case MetaData.TYPE_cnID:
//            case MetaData.TYPE_atID:
//            case MetaData.TYPE_plID:
//            case MetaData.TYPE_geID:
//            case MetaData.TYPE_sfID:
//            case MetaData.TYPE_akID:
//            case MetaData.TYPE_aART:
//            case MetaData.TYPE_TVNN:
//            case MetaData.TYPE_TVSH:
//            case MetaData.TYPE_TVEN:
//            case MetaData.TYPE_TVSN:
//            case MetaData.TYPE_TVES:
//            case MetaData.TYPE_STIK:
                return true;

            default:
                return false;
        }
    }
    
    public MetaDataTypeHandler(AtomFactory atomFactory) {
        this.atomFactory = atomFactory;
    }

    public Atom createAtom(int type, int size, RandomAccessFile source, int context) throws IOException, InvalidFormatException {
        Atom atom = null;
        if (context == Atom.TYPE_ILST) {
            if (isMetaDataType(type)) {
                atomFactory.setContext(type);
                atom = new ContainerAtom(type, size, false, source, atomFactory);
                atomFactory.setContext(context);
            }
        } else if (type == MetaData.TYPE_DATA) {
            if (isMetaDataType(context)) {
                atom = new DataAtom(size, source);
            }
        } else if (context == MetaData.TYPE_dddd) {
            if (type == MetaData.TYPE_MEAN || type == MetaData.TYPE_NAME) {
                atom = new StringAtom(type, size, source);
            }
        }

        return atom;
    }
}
