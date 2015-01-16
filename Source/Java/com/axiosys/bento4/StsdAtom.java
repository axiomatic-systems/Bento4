/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: StsdAtom.java 196 2008-10-14 22:59:31Z bok $
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
import java.util.ArrayList;

public class StsdAtom extends Atom implements AtomParent {
    private ArrayList entries = new ArrayList();

    public StsdAtom(int size, RandomAccessFile source, AtomFactory atomFactory) throws IOException, InvalidFormatException {
        super(TYPE_STSD, size, true, source);
        
        // read the number of entries
        int entryCount = source.readInt();
        int[] bytesAvailable = new int[] { size-FULL_HEADER_SIZE-4 };
        for (int i=0; i<entryCount; i++) {
            Atom atom = atomFactory.createAtom(source, bytesAvailable);
            if (atom != null) {
                entries.add(atom);
            }
        }
    }    
    
    public void writeFields(DataOutputStream stream) throws IOException {
        stream.writeInt(entries.size());
        
        for (int i=0; i<entries.size(); i++) {
            Atom atom = (Atom)entries.get(i);
            atom.write(stream);
        }        
    }
    

    public int getChildrenCount() {
        return entries.size();
    }

    public Atom getChild(int index) {
        return (Atom)entries.get(index);
    }

    public Atom getChild(int type, int index) {
        return AtomUtils.findChild(entries, type, index);
    }

    public String toString(String indentation) {
        StringBuffer result = new StringBuffer();
        result.append(super.toString(indentation));
        for (int i=0; i<entries.size(); i++) {
            result.append("\n");
            result.append(((Atom)entries.get(i)).toString(indentation+"  "));
        }
        
        return result.toString();  
    }
    
    public String toString() {
        return toString("");
    }
}
