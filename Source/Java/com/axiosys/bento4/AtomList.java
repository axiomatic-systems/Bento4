/*****************************************************************
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
|
|    $Id: AtomList.java 196 2008-10-14 22:59:31Z bok $
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

import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;

public class AtomList implements AtomParent {
    private final ArrayList atoms = new ArrayList();
    
    public AtomList(String filename) throws IOException, InvalidFormatException {
        RandomAccessFile input = new RandomAccessFile(filename, "r");
        Atom atom;
        do {
            atom = AtomFactory.DefaultFactory.createAtom(input);
            if (atom != null) atoms.add(atom);
        } while (atom != null);
        //input.close(); do not close the input here as some atoms may need to read from it later
    }
    
    public int getChildrenCount() {
        return atoms.size();
    }

    public Atom getChild(int index) {
        return (Atom)atoms.get(index);
    }

    public Atom getChild(int type, int index) {
        return AtomUtils.findChild(atoms, type, index);
    }
    
    public String toString() {
        StringBuffer result = new StringBuffer();
        String sep = "";
        for (int i=0; i<atoms.size(); i++) {
            Atom atom = (Atom)atoms.get(i);
            result.append(atom.toString() + sep);
            sep = "\n";
        }
        
        return result.toString();
    }
}
